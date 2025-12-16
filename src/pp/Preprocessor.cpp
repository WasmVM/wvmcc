#include "Preprocessor.hpp"
#include "ConstExprParser.hpp"

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include "Tokenizer.hpp"

namespace wvmcc {

// Helper function to stringify tokens (for # operator in macros)
// Now a member method (forward reference removed)

static bool file_ends_with_newline(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return true; // if unreadable, avoid noisy warnings here
    f.seekg(0, std::ios::end);
    std::streampos sz = f.tellg();
    if (sz <= 0) return true; // empty file: treat as ok
    f.seekg(-1, std::ios::end);
    char last = '\n';
    f.read(&last, 1);
    return last == '\n';
}

bool Preprocessor::handleIncludeDirective(Tokenizer& tokenizer,
                                          std::vector<PPToken>& out,
                                          const std::string& currentDir) {
    auto executeInclude = [&](const std::string& header, bool isAngle, std::optional<SourceSpan> span) -> bool {
        if (auto resolved = resolveInclude(header, isAngle, currentDir)) {
            // #pragma once optimization: skip if already processed
            if (pragmaOnceFiles.count(*resolved) > 0) {
                return true; // File has #pragma once and was already included, skip it
            }
            
            Preprocessor child;
            child.includePaths = includePaths;
            child.inclusionStack = inclusionStack;
            child.pragmaOnceFiles = pragmaOnceFiles;  // Share pragma once info
            auto childRes = child.run(*resolved);
            diagnostics.insert(diagnostics.end(), child.diagnostics.begin(), child.diagnostics.end());
            
            // Merge back pragma once files from child
            for (const auto& file : child.pragmaOnceFiles) {
                pragmaOnceFiles.insert(file);
            }
            
            if (childRes.success) {
                for (const auto& tk : childRes.tokens) out.push_back(tk);
                return true;
            }
            diagnostics.push_back(Diagnostic{
                .message = std::string("failed to read include file '") + *resolved + "'",
                .severity = Diagnostic::Severity::Error,
                .span = span
            });
            return false;
        }
        diagnostics.push_back(Diagnostic{
            .message = std::string("include file not found: ") + header,
            .severity = Diagnostic::Severity::Error,
            .span = span
        });
        return false;
    };

    // Skip spaces after include
    while (auto w = tokenizer.peek()) {
        if (w->kind == PPTokenKind::Whitespace) tokenizer.next();
        else break;
    }

    auto tok = tokenizer.peek();
    if (tok && tok->kind == PPTokenKind::Punctuator && tok->lexeme == "<") {
        tokenizer.next();
        std::string header;
        auto begin = tok->span.begin;
        SourcePos end = tok->span.end;
        while (auto t = tokenizer.peek()) {
            if (t->kind == PPTokenKind::Punctuator && t->lexeme == ">") { end = t->span.end; tokenizer.next(); break; }
            if (t->kind == PPTokenKind::Newline) break;
            header += t->lexeme; tokenizer.next();
        }
        return executeInclude(header, /*isAngle=*/true, SourceSpan{begin, end});
    }

    if (tok && tok->kind == PPTokenKind::StringLiteral) {
        auto lit = *tokenizer.next();
        std::string header = lit.lexeme;
        if (header.size() >= 2 && header.front() == '"' && header.back() == '"') {
            header = header.substr(1, header.size() - 2);
        }
        // First: quote search, then fallback to angle with same sequence
        if (executeInclude(header, /*isAngle=*/false, lit.span)) return true;
        return executeInclude(header, /*isAngle=*/true, lit.span);
    }

    // Macro-replaced include: collect, expand, then parse header-name
    auto tail = collectLineTokens(tokenizer);
    auto expanded = expandMacros(tail);
    size_t i = 0;
    while (i < expanded.size() && expanded[i].kind == PPTokenKind::Whitespace) i++;

    if (i >= expanded.size()) {
        diagnostics.push_back(Diagnostic{
            .message = std::string("invalid #include after macro expansion: missing header-name"),
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    if (expanded[i].kind == PPTokenKind::Punctuator && expanded[i].lexeme == "<") {
        std::string header; size_t j = i + 1; int depth = 1;
        SourcePos begin = expanded[i].span.begin; SourcePos end = expanded[i].span.end;
        for (; j < expanded.size(); ++j) {
            if (expanded[j].kind == PPTokenKind::Punctuator && expanded[j].lexeme == "<") depth++;
            else if (expanded[j].kind == PPTokenKind::Punctuator && expanded[j].lexeme == ">") {
                if (--depth == 0) { end = expanded[j].span.end; break; }
            } else if (expanded[j].kind != PPTokenKind::Newline) {
                header += expanded[j].lexeme;
            }
        }
        if (depth == 0) {
            return executeInclude(header, /*isAngle=*/true, SourceSpan{begin, end});
        }
        diagnostics.push_back(Diagnostic{
            .message = std::string("unterminated header-name in macro-expanded #include"),
            .severity = Diagnostic::Severity::Error,
            .span = SourceSpan{begin, expanded.back().span.end}
        });
        return false;
    }

    if (expanded[i].kind == PPTokenKind::StringLiteral) {
        auto lit = expanded[i];
        std::string header = lit.lexeme;
        if (header.size() >= 2 && header.front() == '"' && header.back() == '"') {
            header = header.substr(1, header.size() - 2);
        }
        if (executeInclude(header, /*isAngle=*/false, lit.span)) return true;
        return executeInclude(header, /*isAngle=*/true, lit.span);
    }

    diagnostics.push_back(Diagnostic{
        .message = std::string("invalid #include after macro expansion: expected <header> or \"header\""),
        .severity = Diagnostic::Severity::Error,
        .span = expanded[i].span
    });
    return false;
}

std::optional<std::string> Preprocessor::resolveInclude(const std::string& header,
                                                        bool isAngle,
                                                        const std::string& currentDir) const {
    namespace fs = std::filesystem;
    auto existsFile = [](const fs::path& p) -> bool {
        std::error_code ec;
        return fs::exists(p, ec) && fs::is_regular_file(p, ec);
    };

    // Quote includes: current file dir first
    if (!isAngle) {
        if (!currentDir.empty()) {
            fs::path p = fs::path(currentDir) / header;
            if (existsFile(p)) return fs::weakly_canonical(p).string();
        }
    }

    // Both forms: search -I paths
    for (const auto& base : includePaths) {
        fs::path p = fs::path(base) / header;
        if (existsFile(p)) return fs::weakly_canonical(p).string();
    }

    // As a fallback for quotes, try relative to CWD if no currentDir
    if (!isAngle && currentDir.empty()) {
        fs::path p = fs::path(header);
        if (existsFile(p)) return fs::weakly_canonical(p).string();
    }

    return std::nullopt;
}

bool Preprocessor::isInInclusionStack(const std::string& filePath) const {
    return std::find(inclusionStack.begin(), inclusionStack.end(), filePath) 
        != inclusionStack.end();
}

bool Preprocessor::pushInclusion(const std::string& filePath) {
    if (isInInclusionStack(filePath)) return false;  // Cycle detected
    inclusionStack.push_back(filePath);
    return true;
}

void Preprocessor::popInclusion() {
    if (!inclusionStack.empty()) inclusionStack.pop_back();
}

PreprocessResult Preprocessor::run(const std::string& inputPath) {
    std::ifstream ifs(inputPath);
    if (!ifs) {
        return PreprocessResult{std::vector<wvmcc::PPToken>{}, false, std::string("cannot open input: ") + inputPath};
    }
    
    // Canonicalize path and check for cycles
    namespace fs = std::filesystem;
    std::string canonicalPath = fs::weakly_canonical(fs::absolute(inputPath)).string();
    if (!pushInclusion(canonicalPath)) {
        // Cycle detected
        diagnostics.push_back(Diagnostic{
            .message = std::string("cyclic include detected: ") + canonicalPath,
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return PreprocessResult{std::vector<wvmcc::PPToken>{}, false, std::string("cyclic include: ") + canonicalPath};
    }
    
    // Initialize predefined macros for this file (only once per run)
    if (inclusionStack.size() == 1) {
        // __FILE__ macro - current source file
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::StringLiteral,
                .span = {},
                .lexeme = "\"" + canonicalPath + "\"",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__FILE__", replacement);
        }
        
        // __LINE__ macro is handled dynamically during expansion (not predefined)
        
        // __DATE__ macro - compilation date
        {
            auto now = std::time(nullptr);
            auto tm = *std::localtime(&now);
            char dateStr[32];
            std::strftime(dateStr, sizeof(dateStr), "%b %d %Y", &tm);
            
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::StringLiteral,
                .span = {},
                .lexeme = "\"" + std::string(dateStr) + "\"",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__DATE__", replacement);
        }
        
        // __TIME__ macro - compilation time
        {
            auto now = std::time(nullptr);
            auto tm = *std::localtime(&now);
            char timeStr[32];
            std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
            
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::StringLiteral,
                .span = {},
                .lexeme = "\"" + std::string(timeStr) + "\"",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__TIME__", replacement);
        }
        
        // __STDC__ macro - ISO C compliance (set to 1)
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC__", replacement);
        }
        
        // __STDC_VERSION__ macro - C standard version (C17 = 201710L)
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "201710L",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_VERSION__", replacement);
        }
        
        // __STDC_HOSTED__ macro - freestanding implementation (0 = freestanding, 1 = hosted)
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "0",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_HOSTED__", replacement);
        }
        
        // __STDC_NO_ATOMICS__ macro - no atomic support
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_NO_ATOMICS__", replacement);
        }
        
        // __STDC_NO_COMPLEX__ macro - no complex number support
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_NO_COMPLEX__", replacement);
        }
        
        // __STDC_NO_THREADS__ macro - no threading support
        {
            std::vector<PPToken> replacement;
            replacement.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = {},
                .lexeme = "1",
                .paintedMacros = {}
            });
            macroTable.defineObjectMacro("__STDC_NO_THREADS__", replacement);
        }
    }
    
    // Cleanup: pop inclusion on exit
    auto popGuard = [this]() { popInclusion(); };
    // Simplified: use a lambda immediately after scope, or just call at the end
    
    const bool endsWithNewline = file_ends_with_newline(inputPath);
    // Derive current file directory for quote-style includes
    std::filesystem::path inputPathFs(inputPath);
    std::string currentDir = inputPathFs.has_parent_path() ? inputPathFs.parent_path().string() : std::string();
    // Single-pass streaming: consume tokens, parse directives and includes
    Tokenizer tokenizer(ifs);
    std::vector<PPToken> out;
    out.reserve(256);
    tokenizer.reset();
    bool atLineStart = true; // start of file is line start
    bool hasErrors = false;  // Track errors during preprocessing
    while (auto tokOpt = tokenizer.next()) {
        PPToken t = *tokOpt;
        // Basic validation for literals on the fly
        if (t.kind == PPTokenKind::StringLiteral) {
            if (t.lexeme.empty() || t.lexeme.back() != '"') {
                std::ostringstream em;
                em << "unterminated string literal at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
                popInclusion();
                return PreprocessResult{std::vector<PPToken>{}, false, em.str()};
            }
        } else if (t.kind == PPTokenKind::CharConst) {
            if (t.lexeme.empty() || t.lexeme.back() != '\'' ) {
                std::ostringstream em;
                em << "unterminated character constant at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
                popInclusion();
                return PreprocessResult{std::vector<PPToken>{}, false, em.str()};
            }
        }

        if (t.kind == PPTokenKind::Newline) {
            // Emit newlines even in inactive regions to maintain line count
            out.push_back(t);
            atLineStart = true;
            continue;
        }
        if (atLineStart) {
            if (t.kind == PPTokenKind::Whitespace) {
                // Emit whitespace at line start regardless of conditional state
                out.push_back(t);
                // still at line start until a non-whitespace token
                continue;
            }
            if (t.kind == PPTokenKind::Punctuator && t.lexeme == "#") {
                // Don't push '#' yet; we'll decide later if this is a recognized directive
                // Skip spaces (do not emit) between '#' and directive keyword
                while (auto p = tokenizer.peek()) {
                    if (p->kind == PPTokenKind::Whitespace) { tokenizer.next(); }
                    else break;
                }
                auto dir = tokenizer.peek();
                if (dir && dir->kind == PPTokenKind::Identifier) {
                    // Consume directive keyword (don't push yet)
                    tokenizer.next();
                    // Special handling for include header-name recognition
                    if (dir->lexeme == "include") {
                        // For executed includes, do not emit '#' or 'include' tokens
                        // Only execute includes if we're in an active conditional region
                        bool shouldExecute = conditionalStack.empty() || conditionalStack.back().currentlyActive;
                        bool executed = shouldExecute && handleIncludeDirective(tokenizer, out, currentDir);
                        if (!executed) {
                            // Check if include failed due to error
                            bool hasErrorDiag = false;
                            for (const auto& d : diagnostics) {
                                if (d.severity == Diagnostic::Severity::Error) {
                                    hasErrorDiag = true;
                                    break;
                                }
                            }
                            if (hasErrorDiag) {
                                hasErrors = true;
                            } else {
                                // Not executed and no error: emit '#' and 'include' as part of directive line
                                out.push_back(t);
                                out.push_back(*dir);
                            }
                        }
                    } else {
                        // Other directives: parse and execute
                        if (dir->lexeme == "define") {
                            // Only execute #define if in active region
                            bool shouldExecute = conditionalStack.empty() || conditionalStack.back().currentlyActive;
                            if (shouldExecute) {
                                if (!handleDefineDirective(tokenizer)) {
                                    hasErrors = true;
                                }
                            } else {
                                // Skip directive in inactive region
                                while (auto a = tokenizer.peek()) {
                                    if (a->kind == PPTokenKind::Newline) break;
                                    tokenizer.next();
                                }
                            }
                        } else if (dir->lexeme == "undef") {
                            // Only execute #undef if in active region
                            bool shouldExecute = conditionalStack.empty() || conditionalStack.back().currentlyActive;
                            if (shouldExecute) {
                                if (!handleUndefDirective(tokenizer)) {
                                    hasErrors = true;
                                }
                            } else {
                                // Skip directive in inactive region
                                while (auto a = tokenizer.peek()) {
                                    if (a->kind == PPTokenKind::Newline) break;
                                    tokenizer.next();
                                }
                            }
                        } else if (dir->lexeme == "if") {
                            auto lineTokens = collectLineTokens(tokenizer);
                            if (!handleIfDirective(tokenizer, lineTokens)) {
                                hasErrors = true;
                            }
                        } else if (dir->lexeme == "ifdef") {
                            auto lineTokens = collectLineTokens(tokenizer);
                            if (lineTokens.empty()) {
                                diagnostics.push_back(Diagnostic{
                                    .message = "expected macro name after #ifdef",
                                    .severity = Diagnostic::Severity::Error,
                                    .span = std::nullopt
                                });
                                hasErrors = true;
                            } else {
                                // Find identifier in line tokens (skip whitespace)
                                std::string macroName;
                                for (const auto& lt : lineTokens) {
                                    if (lt.kind == PPTokenKind::Identifier) {
                                        macroName = lt.lexeme;
                                        break;
                                    }
                                }
                                if (macroName.empty()) {
                                    diagnostics.push_back(Diagnostic{
                                        .message = "expected macro name after #ifdef",
                                        .severity = Diagnostic::Severity::Error,
                                        .span = std::nullopt
                                    });
                                    hasErrors = true;
                                } else {
                                    if (!handleIfdefDirective(macroName)) {
                                        hasErrors = true;
                                    }
                                }
                            }
                        } else if (dir->lexeme == "ifndef") {
                            auto lineTokens = collectLineTokens(tokenizer);
                            if (lineTokens.empty()) {
                                diagnostics.push_back(Diagnostic{
                                    .message = "expected macro name after #ifndef",
                                    .severity = Diagnostic::Severity::Error,
                                    .span = std::nullopt
                                });
                                hasErrors = true;
                            } else {
                                // Find identifier in line tokens (skip whitespace)
                                std::string macroName;
                                for (const auto& lt : lineTokens) {
                                    if (lt.kind == PPTokenKind::Identifier) {
                                        macroName = lt.lexeme;
                                        break;
                                    }
                                }
                                if (macroName.empty()) {
                                    diagnostics.push_back(Diagnostic{
                                        .message = "expected macro name after #ifndef",
                                        .severity = Diagnostic::Severity::Error,
                                        .span = std::nullopt
                                    });
                                    hasErrors = true;
                                } else {
                                    if (!handleIfndefDirective(macroName)) {
                                        hasErrors = true;
                                    }
                                }
                            }
                        } else if (dir->lexeme == "elif") {
                            auto lineTokens = collectLineTokens(tokenizer);
                            if (!handleElifDirective(tokenizer, lineTokens)) {
                                hasErrors = true;
                            }
                        } else if (dir->lexeme == "else") {
                            if (!handleElseDirective()) {
                                hasErrors = true;
                            }
                        } else if (dir->lexeme == "endif") {
                            if (!handleEndifDirective()) {
                                hasErrors = true;
                            }
                        } else if (dir->lexeme == "error") {
                            // Only execute #error if in active region
                            bool shouldExecute = conditionalStack.empty() || conditionalStack.back().currentlyActive;
                            if (shouldExecute) {
                                auto lineTokens = collectLineTokens(tokenizer);
                                if (!handleErrorDirective(lineTokens)) {
                                    hasErrors = true;
                                }
                            } else {
                                // Consume tokens but don't execute
                                collectLineTokens(tokenizer);
                            }
                        } else if (dir->lexeme == "warning") {
                            // Only execute #warning if in active region
                            bool shouldExecute = conditionalStack.empty() || conditionalStack.back().currentlyActive;
                            if (shouldExecute) {
                                auto lineTokens = collectLineTokens(tokenizer);
                                if (!handleWarningDirective(lineTokens)) {
                                    hasErrors = true;
                                }
                            } else {
                                // Consume tokens but don't execute
                                collectLineTokens(tokenizer);
                            }
                        } else if (dir->lexeme == "line") {
                            // Only execute #line if in active region
                            bool shouldExecute = conditionalStack.empty() || conditionalStack.back().currentlyActive;
                            if (shouldExecute) {
                                auto lineTokens = collectLineTokens(tokenizer);
                                if (!handleLineDirective(lineTokens)) {
                                    hasErrors = true;
                                }
                            } else {
                                // Consume tokens but don't execute
                                collectLineTokens(tokenizer);
                            }
                        } else if (dir->lexeme == "pragma") {
                            // Only execute #pragma if in active region
                            bool shouldExecute = conditionalStack.empty() || conditionalStack.back().currentlyActive;
                            if (shouldExecute) {
                                auto lineTokens = collectLineTokens(tokenizer);
                                if (!handlePragmaDirective(lineTokens)) {
                                    hasErrors = true;
                                }
                            } else {
                                // Consume tokens but don't execute
                                collectLineTokens(tokenizer);
                            }
                        } else {
                            // Unknown directive: consume until newline but emit as-is
                            while (auto a = tokenizer.peek()) {
                                if (a->kind == PPTokenKind::Newline) break;
                                out.push_back(*tokenizer.next());
                            }
                        }
                    }
                }
                // We're in a directive context; remain not at line start until newline
                atLineStart = false;
                continue;
            }
            // Non-directive non-whitespace at line start
            if (conditionalStack.empty() || conditionalStack.back().currentlyActive) {
                out.push_back(t);
            }
            atLineStart = false;
            continue;
        }
        // Normal token outside of line start
        if (conditionalStack.empty() || conditionalStack.back().currentlyActive) {
            out.push_back(t);
        }
    }

    if (!endsWithNewline) {
        diagnostics.push_back(Diagnostic{
            .message = std::string("input file '") + inputPath + "' does not end with a newline",
            .severity = Diagnostic::Severity::Warning,
            .span = std::nullopt
        });
    }
    
    // Check for unclosed conditional directives
    if (!conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#if/#ifdef/#ifndef without matching #endif",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        hasErrors = true;
    }
    
    // TODO: Include guard detection will be implemented properly
    // For now, the optimization is disabled to avoid incorrect behavior
    
    popInclusion();
    
    // Return failure if any errors were encountered during preprocessing
    if (hasErrors) {
        return PreprocessResult{std::vector<PPToken>{}, false, std::string("preprocessing failed with errors")};
    }

    // Optional debug path: skip macro expansion when PP_DEBUG_NO_EXPAND is set.
    if (std::getenv("PP_DEBUG_NO_EXPAND")) {
        return PreprocessResult{out, true, std::string()};
    }

    // Expand macros in the token stream
    std::vector<PPToken> expanded = expandMacros(out);

    return PreprocessResult{std::move(expanded), true, std::string()};
}

std::vector<PPToken> Preprocessor::collectLineTokens(Tokenizer& tokenizer) {
    std::vector<PPToken> tokens;
    while (auto t = tokenizer.peek()) {
        if (t->kind == PPTokenKind::Newline) {
            break;
        }
        tokens.push_back(*tokenizer.next());
    }
    return tokens;
}

bool Preprocessor::handleDefineDirective(Tokenizer& tokenizer) {
    // Skip whitespace after 'define' keyword
    while (auto w = tokenizer.peek()) {
        if (w->kind == PPTokenKind::Whitespace) { tokenizer.next(); }
        else break;
    }

    auto macroName = tokenizer.peek();
    if (!macroName || macroName->kind != PPTokenKind::Identifier) {
        diagnostics.push_back(Diagnostic{
            .message = "expected macro name after #define",
            .severity = Diagnostic::Severity::Error,
        });
        return false;
    }

    std::string name = macroName->lexeme;
    tokenizer.next(); // consume macro name

    // Check if this is a function-like or object-like macro
    auto next = tokenizer.peek();
    bool isFunction = false;
    std::vector<std::string> params;
    bool variadic = false;

    if (next && next->kind == PPTokenKind::Punctuator && next->lexeme == "(") {
        // Function-like macro: #define NAME(params) replacement
        // Note: no space between NAME and (
        isFunction = true;
        tokenizer.next(); // consume '('

        // Parse parameter list
        while (auto param = tokenizer.peek()) {
            if (param->kind == PPTokenKind::Punctuator && param->lexeme == ")") {
                tokenizer.next(); // consume ')'
                break;
            }
            if (param->kind == PPTokenKind::Whitespace) {
                tokenizer.next();
                continue;
            }
            if (param->kind == PPTokenKind::Punctuator && param->lexeme == ",") {
                tokenizer.next();
                continue;
            }
            if (param->kind == PPTokenKind::Punctuator && param->lexeme == "...") {
                variadic = true;
                tokenizer.next();
                // consume trailing ) for variadic
                while (auto p = tokenizer.peek()) {
                    if (p->kind == PPTokenKind::Whitespace) { tokenizer.next(); }
                    else break;
                }
                if (auto close = tokenizer.peek()) {
                    if (close->kind == PPTokenKind::Punctuator && close->lexeme == ")") {
                        tokenizer.next();
                    }
                }
                break;
            }
            if (param->kind == PPTokenKind::Identifier) {
                params.push_back(param->lexeme);
                tokenizer.next();
            } else {
                diagnostics.push_back(Diagnostic{
                    .message = "invalid parameter in function-like macro",
                    .severity = Diagnostic::Severity::Error,
                });
                return false;
            }
        }
    }

    // Collect replacement tokens until end of line
    std::vector<PPToken> replacement = collectLineTokens(tokenizer);

    // Remove leading/trailing whitespace from replacement
    while (!replacement.empty() && replacement.front().kind == PPTokenKind::Whitespace) {
        replacement.erase(replacement.begin());
    }
    while (!replacement.empty() && replacement.back().kind == PPTokenKind::Whitespace) {
        replacement.pop_back();
    }

    // Store macro definition
    if (isFunction) {
        macroTable.defineFunctionMacro(name, params, replacement, variadic);
    } else {
        macroTable.defineObjectMacro(name, replacement);
    }

    return true;
}

bool Preprocessor::handleUndefDirective(Tokenizer& tokenizer) {
    // Skip whitespace after 'undef' keyword
    while (auto w = tokenizer.peek()) {
        if (w->kind == PPTokenKind::Whitespace) { tokenizer.next(); }
        else break;
    }

    auto macroName = tokenizer.peek();
    if (!macroName || macroName->kind != PPTokenKind::Identifier) {
        diagnostics.push_back(Diagnostic{
            .message = "expected macro name after #undef",
            .severity = Diagnostic::Severity::Error,
        });
        return false;
    }

    std::string name = macroName->lexeme;
    macroTable.undefine(name);
    tokenizer.next(); // consume macro name

    return true;
}

std::string Preprocessor::stringifyTokens(const std::vector<PPToken>& tokens) {
    std::string result = "\"";
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        
        // Skip leading whitespace
        if (i == 0 && tok.kind == PPTokenKind::Whitespace) continue;
        
        // Skip whitespace unless needed for separation
        if (tok.kind == PPTokenKind::Whitespace) {
            // Only add space if previous token was not punctuation and next is not punctuation
            if (i > 0 && i + 1 < tokens.size() &&
                tokens[i-1].kind != PPTokenKind::Punctuator &&
                tokens[i+1].kind != PPTokenKind::Punctuator) {
                result += " ";
            }
            continue;
        }
        
        // Escape quotes and backslashes in the stringified content
        std::string lexeme = tok.lexeme;
        for (char c : lexeme) {
            if (c == '"' || c == '\\') {
                result += '\\';
            }
            result += c;
        }
    }
    
    result += "\"";
    return result;
}

std::vector<std::vector<PPToken>> Preprocessor::collectMacroArguments(
    const std::vector<PPToken>& tokens, size_t startIdx, size_t& endIdx, const Macro* m) {
    
    std::vector<std::vector<PPToken>> args;
    std::vector<PPToken> currentArg;
    int parenDepth = 1;  // Start with depth 1 since we're already inside the opening '('
    
    for (size_t k = startIdx; k < tokens.size(); ++k) {
        const auto& tok = tokens[k];
        
        if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "(") {
            parenDepth++;
            currentArg.push_back(tok);
        } else if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == ")") {
            parenDepth--;
            if (parenDepth == 0) {
                // End of arguments
                if (!currentArg.empty() || args.empty()) {
                    args.push_back(currentArg);
                }
                endIdx = k;
                break;
            } else {
                currentArg.push_back(tok);
            }
        } else if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "," && parenDepth == 1) {
            // Argument separator at the top level
            args.push_back(currentArg);
            currentArg.clear();
        } else if (tok.kind != PPTokenKind::Whitespace || !currentArg.empty()) {
            currentArg.push_back(tok);
        }
    }
    
    // Handle variadic macros - collect remaining args as __VA_ARGS__
    if (m->variadic && args.size() >= m->params.size()) {
        std::vector<PPToken> variadicArg;
        for (size_t vIdx = m->params.size(); vIdx < args.size(); ++vIdx) {
            if (vIdx > m->params.size()) {
                variadicArg.push_back(PPToken{
                    .kind = PPTokenKind::Punctuator,
                    .lexeme = ",",
                    .span = {}
                });
            }
            for (const auto& tok : args[vIdx]) {
                variadicArg.push_back(tok);
            }
        }
        args.resize(m->params.size());
        args.push_back(variadicArg);
    }
    
    return args;
}

std::vector<PPToken> Preprocessor::substituteParameters(
    const Macro* m, const std::vector<std::vector<PPToken>>& args, const PPToken& invocationToken) {
    
    std::vector<PPToken> substituted;
    
    for (size_t rIdx = 0; rIdx < m->replacement.size(); ++rIdx) {
        const auto& repl = m->replacement[rIdx];
        bool handled = false;
        
        // Handle stringification operator (#)
        if (repl.kind == PPTokenKind::Punctuator && repl.lexeme == "#") {
            size_t nextIdx = rIdx + 1;
            while (nextIdx < m->replacement.size() && 
                   m->replacement[nextIdx].kind == PPTokenKind::Whitespace) {
                nextIdx++;
            }
            
            if (nextIdx < m->replacement.size()) {
                const auto& nextTok = m->replacement[nextIdx];
                int paramIdx = -1;
                bool isVarargs = (m->variadic && nextTok.kind == PPTokenKind::Identifier && 
                                 nextTok.lexeme == "__VA_ARGS__");
                
                if (!isVarargs) {
                    for (size_t pIdx = 0; pIdx < m->params.size(); ++pIdx) {
                        if (nextTok.kind == PPTokenKind::Identifier && 
                            nextTok.lexeme == m->params[pIdx]) {
                            paramIdx = pIdx;
                            break;
                        }
                    }
                }
                
                if (paramIdx >= 0 || isVarargs) {
                    std::vector<PPToken> argToStringify;
                    if (isVarargs && args.size() > m->params.size()) {
                        argToStringify = args[m->params.size()];
                    } else if (paramIdx >= 0 && paramIdx < static_cast<int>(args.size())) {
                        argToStringify = args[paramIdx];
                    }
                    
                    std::string stringified = stringifyTokens(argToStringify);
                    PPToken strToken{
                        .kind = PPTokenKind::StringLiteral,
                        .span = repl.span,
                        .lexeme = stringified,
                        .paintedMacros = {}
                    };
                    strToken.paint(m->name);
                    substituted.push_back(strToken);
                    rIdx = nextIdx;
                    handled = true;
                }
            }
        }
        
        // Handle __VA_ARGS__
        if (!handled && m->variadic && repl.kind == PPTokenKind::Identifier && 
            repl.lexeme == "__VA_ARGS__") {
            if (args.size() > m->params.size()) {
                for (auto vaToken : args[m->params.size()]) {
                    vaToken.paint(m->name);
                    substituted.push_back(vaToken);
                }
            }
            handled = true;
        }
        
        // Handle regular parameters
        if (!handled) {
            for (size_t pIdx = 0; pIdx < m->params.size(); ++pIdx) {
                if (repl.kind == PPTokenKind::Identifier && repl.lexeme == m->params[pIdx]) {
                    if (pIdx < args.size()) {
                        for (auto arg : args[pIdx]) {
                            arg.paint(m->name);
                            substituted.push_back(arg);
                        }
                    }
                    handled = true;
                    break;
                }
            }
        }
        
        // Not a parameter - copy token and paint it
        if (!handled) {
            auto replToken = repl;
            replToken.paint(m->name);
            // Special handling for __LINE__: update span to invocation site
            if (replToken.kind == PPTokenKind::Identifier && replToken.lexeme == "__LINE__") {
                replToken.span = invocationToken.span;
            }
            substituted.push_back(replToken);
        }
    }
    
    return substituted;
}

std::vector<PPToken> Preprocessor::handleTokenPasting(const std::vector<PPToken>& tokens) {
    std::vector<PPToken> result;
    
    for (size_t idx = 0; idx < tokens.size(); ++idx) {
        const auto& tok = tokens[idx];
        
        // Check if current token is ##
        if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "##") {
            // Find previous non-whitespace token
            int prevIdx = idx - 1;
            while (prevIdx >= 0 && tokens[prevIdx].kind == PPTokenKind::Whitespace) {
                prevIdx--;
            }
            
            // Find next non-whitespace token
            int nextIdx = idx + 1;
            while (nextIdx < static_cast<int>(tokens.size()) && 
                   tokens[nextIdx].kind == PPTokenKind::Whitespace) {
                nextIdx++;
            }
            
            // Paste tokens if we have both previous and next
            if (prevIdx >= 0 && nextIdx < static_cast<int>(tokens.size())) {
                // Remove whitespace before paste operator
                while (!result.empty() && result.back().kind == PPTokenKind::Whitespace) {
                    result.pop_back();
                }
                
                // Paste: concatenate previous and next token lexemes
                if (!result.empty()) {
                    result.back().lexeme += tokens[nextIdx].lexeme;
                    idx = nextIdx;  // Skip to next token
                    continue;
                }
            }
        }
        
        result.push_back(tok);
    }
    
    return result;
}

bool Preprocessor::tryExpandFunctionLikeMacro(
    const std::vector<PPToken>& tokens, size_t& i, const Macro* m, 
    const PPToken& invocationToken, std::vector<PPToken>& result) {
    
    // Find opening '('
    size_t j = i + 1;
    while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) {
        j++;
    }
    
    // Check if we have '(' - if not, don't expand
    if (j >= tokens.size() || tokens[j].kind != PPTokenKind::Punctuator ||
        tokens[j].lexeme != "(") {
        return false;
    }
    
    // Collect arguments
    size_t endIdx = 0;
    auto args = collectMacroArguments(tokens, j + 1, endIdx, m);
    
    // Validate argument count
    if (!m->variadic && args.size() != m->params.size()) {
        return false;
    }
    if (m->variadic && args.size() < m->params.size()) {
        return false;
    }
    
    // Substitute parameters
    auto substituted = substituteParameters(m, args, invocationToken);
    
    // Handle token pasting
    auto afterPasting = handleTokenPasting(substituted);
    
    // Recursively expand
    auto expanded = expandMacros(afterPasting);
    for (const auto& exp : expanded) {
        result.push_back(exp);
    }
    
    i = endIdx;  // Update position past the closing ')'
    return true;
}

std::vector<PPToken> Preprocessor::expandMacros(const std::vector<PPToken>& tokens) {
    // Paint semantics implementation (C17 §6.10.3.3):
    // Each token tracks a "paint set" of macro names that have already tried to expand it.
    // This prevents recursion while allowing legitimate multi-level expansions.
    
    std::vector<PPToken> result;

    for (size_t i = 0; i < tokens.size(); ++i) {
        auto t = tokens[i];  // Copy to modify paint set

        // Skip expansion for non-identifiers
        if (t.kind != PPTokenKind::Identifier) {
            result.push_back(t);
            continue;
        }

        // Special handling for __LINE__ macro (dynamic expansion)
        if (t.lexeme == "__LINE__") {
            std::string lineStr = std::to_string(t.span.begin.line);
            result.push_back(PPToken{
                .kind = PPTokenKind::PPNumber,
                .span = t.span,
                .lexeme = lineStr,
                .paintedMacros = {}
            });
            continue;
        }

        // Check if this identifier is a defined macro
        auto macro = macroTable.getMacro(t.lexeme);
        if (!macro) {
            result.push_back(t);
            continue;
        }

        const Macro* m = *macro;

        // Skip if this macro is painted (already tried to expand it)
        if (t.isPainted(m->name)) {
            result.push_back(t);
            continue;
        }

        // Try function-like macro expansion
        if (m->isFunction) {
            if (tryExpandFunctionLikeMacro(tokens, i, m, t, result)) {
                continue;
            }
            // If not expanded (no '(' found), treat as regular identifier
            result.push_back(t);
            continue;
        }

        // Object-like macro expansion
        std::vector<PPToken> substituted;
        for (auto repl : m->replacement) {
            repl.paint(m->name);
            if (repl.kind == PPTokenKind::Identifier && repl.lexeme == "__LINE__") {
                repl.span = t.span;  // Update to invocation site
            }
            substituted.push_back(repl);
        }

        // Recursively expand the substituted tokens
        auto expanded = expandMacros(substituted);
        for (const auto& exp : expanded) {
            result.push_back(exp);
        }
    }

    return result;
}

std::optional<int64_t> Preprocessor::evaluateConstantExpression(const std::vector<PPToken>& tokens) {
    // First, fold defined-operator operands to 0/1 without expanding the operand macro name.
    std::vector<PPToken> preprocessed;
    preprocessed.reserve(tokens.size());
    for (size_t i = 0; i < tokens.size();) {
        const auto& tok = tokens[i];
        if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "defined") {
            size_t j = i + 1;
            while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) j++;

            bool hasParens = false;
            if (j < tokens.size() && tokens[j].kind == PPTokenKind::Punctuator && tokens[j].lexeme == "(") {
                hasParens = true;
                ++j;
                while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) j++;
            }

            if (j >= tokens.size() || tokens[j].kind != PPTokenKind::Identifier) {
                diagnostics.push_back(Diagnostic{
                    .message = "expected macro name in 'defined'",
                    .severity = Diagnostic::Severity::Error,
                    .span = (j < tokens.size()) ? std::optional<SourceSpan>(tokens[j].span) : std::optional<SourceSpan>()
                });
                return std::nullopt;
            }

            std::string macroName = tokens[j].lexeme;
            ++j;

            if (hasParens) {
                while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) j++;
                if (j >= tokens.size() || tokens[j].kind != PPTokenKind::Punctuator || tokens[j].lexeme != ")") {
                    diagnostics.push_back(Diagnostic{
                        .message = "expected ')' after macro name in 'defined'",
                        .severity = Diagnostic::Severity::Error,
                        .span = (j < tokens.size()) ? std::optional<SourceSpan>(tokens[j].span) : std::optional<SourceSpan>()
                    });
                    return std::nullopt;
                }
                ++j;
            }

            PPToken folded = tok;
            folded.kind = PPTokenKind::PPNumber;
            folded.lexeme = macroTable.isDefined(macroName) ? "1" : "0";
            preprocessed.push_back(folded);
            i = j;
            continue;
        }

        preprocessed.push_back(tok);
        ++i;
    }

    auto expanded = expandMacros(preprocessed);
    ConstExprParser parser(macroTable, diagnostics);
    return parser.evaluate(expanded);
}

bool Preprocessor::handleIfDirective(Tokenizer& tokenizer, const std::vector<PPToken>& dirTokens) {
    auto result = evaluateConstantExpression(dirTokens);
    if (!result) return false;

    ConditionalFrame frame;
    frame.seenTrueBranch = *result != 0;
    frame.currentlyActive = frame.seenTrueBranch;
    frame.inElse = false;
    conditionalStack.push_back(frame);
    return true;
}

bool Preprocessor::handleIfdefDirective(const std::string& macroName) {
    bool isDefined = macroTable.isDefined(macroName);
    ConditionalFrame frame;
    frame.seenTrueBranch = isDefined;
    frame.currentlyActive = isDefined;
    frame.inElse = false;
    conditionalStack.push_back(frame);
    return true;
}

bool Preprocessor::handleIfndefDirective(const std::string& macroName) {
    if (std::getenv("PP_DEBUG_IFNDEF")) {
        std::cerr << "[pp] #ifndef " << macroName << " defined=" << (macroTable.isDefined(macroName) ? "1" : "0")
                  << " macros=" << macroTable.count() << "\n";
    }
    bool isDefined = macroTable.isDefined(macroName);
    ConditionalFrame frame;
    frame.seenTrueBranch = !isDefined;
    frame.currentlyActive = !isDefined;
    frame.inElse = false;
    conditionalStack.push_back(frame);
    return true;
}

bool Preprocessor::handleElifDirective(Tokenizer& tokenizer, const std::vector<PPToken>& dirTokens) {
    if (conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#elif without matching #if",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    ConditionalFrame& frame = conditionalStack.back();
    if (frame.inElse) {
        diagnostics.push_back(Diagnostic{
            .message = "#elif after #else",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    if (frame.seenTrueBranch) {
        frame.currentlyActive = false;
        return true;
    }

    auto result = evaluateConstantExpression(dirTokens);
    if (!result) return false;

    bool newActive = *result != 0;
    frame.seenTrueBranch = frame.seenTrueBranch || newActive;
    frame.currentlyActive = newActive;
    return true;
}

bool Preprocessor::handleElseDirective() {
    if (conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#else without matching #if",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    ConditionalFrame& frame = conditionalStack.back();
    if (frame.inElse) {
        diagnostics.push_back(Diagnostic{
            .message = "multiple #else in same conditional block",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    frame.inElse = true;
    frame.currentlyActive = !frame.seenTrueBranch;
    return true;
}

bool Preprocessor::handleEndifDirective() {
    if (conditionalStack.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#endif without matching #if",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }

    conditionalStack.pop_back();
    return true;
}

bool Preprocessor::handleErrorDirective(const std::vector<PPToken>& tokens) {
    // #error directive: emit error message and fail preprocessing
    // Collect all tokens (excluding whitespace at boundaries) as the error message
    std::string errorMsg;
    
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Whitespace) {
            if (!errorMsg.empty()) {
                errorMsg += " ";
            }
        } else {
            errorMsg += tok.lexeme;
        }
    }
    
    // Trim trailing whitespace
    while (!errorMsg.empty() && (errorMsg.back() == ' ' || errorMsg.back() == '\t')) {
        errorMsg.pop_back();
    }
    
    diagnostics.push_back(Diagnostic{
        .message = std::string("#error: ") + (errorMsg.empty() ? "(no message)" : errorMsg),
        .severity = Diagnostic::Severity::Error,
        .span = tokens.empty() ? std::nullopt : std::optional<SourceSpan>(tokens[0].span)
    });
    
    return false; // #error always causes preprocessing to fail
}

bool Preprocessor::handleWarningDirective(const std::vector<PPToken>& tokens) {
    // #warning directive: emit warning message but continue preprocessing
    std::string warningMsg;
    
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Whitespace) {
            if (!warningMsg.empty()) {
                warningMsg += " ";
            }
        } else {
            warningMsg += tok.lexeme;
        }
    }
    
    // Trim trailing whitespace
    while (!warningMsg.empty() && (warningMsg.back() == ' ' || warningMsg.back() == '\t')) {
        warningMsg.pop_back();
    }
    
    diagnostics.push_back(Diagnostic{
        .message = std::string("#warning: ") + (warningMsg.empty() ? "(no message)" : warningMsg),
        .severity = Diagnostic::Severity::Warning,
        .span = tokens.empty() ? std::nullopt : std::optional<SourceSpan>(tokens[0].span)
    });
    
    return true; // #warning does not cause preprocessing to fail
}

bool Preprocessor::handleLineDirective(const std::vector<PPToken>& tokens) {
    // #line directive: change line number and optionally filename for subsequent diagnostics
    // Format: #line digit-sequence ["filename"]
    // For now, we'll just validate and accept it without changing our line tracking
    // (Full implementation would require modifying SourcePos in emitted tokens)
    
    if (tokens.empty()) {
        diagnostics.push_back(Diagnostic{
            .message = "#line directive requires line number",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return false;
    }
    
    // First token should be a number
    size_t idx = 0;
    while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) {
        idx++;
    }
    
    if (idx >= tokens.size() || tokens[idx].kind != PPTokenKind::PPNumber) {
        diagnostics.push_back(Diagnostic{
            .message = "#line directive requires line number as first argument",
            .severity = Diagnostic::Severity::Error,
            .span = idx < tokens.size() ? std::optional<SourceSpan>(tokens[idx].span) : std::nullopt
        });
        return false;
    }
    
    // Optionally followed by a string literal (filename)
    idx++;
    while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) {
        idx++;
    }
    
    if (idx < tokens.size()) {
        if (tokens[idx].kind != PPTokenKind::StringLiteral) {
            diagnostics.push_back(Diagnostic{
                .message = "#line directive optional second argument must be a string literal",
                .severity = Diagnostic::Severity::Error,
                .span = tokens[idx].span
            });
            return false;
        }
    }
    
    // Note: Full implementation would update line/file tracking here
    // For now, we just validate the syntax
    return true;
}

bool Preprocessor::handlePragmaDirective(const std::vector<PPToken>& tokens) {
    // #pragma directive: implementation-defined behavior
    // Special handling for #pragma once
    
    // Check if this is #pragma once
    bool isPragmaOnce = false;
    if (!tokens.empty()) {
        size_t idx = 0;
        // Skip leading whitespace
        while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) idx++;
        
        if (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Identifier && 
            tokens[idx].lexeme == "once") {
            // Check there's nothing after "once" (except whitespace)
            idx++;
            while (idx < tokens.size() && tokens[idx].kind == PPTokenKind::Whitespace) idx++;
            
            if (idx >= tokens.size()) {
                isPragmaOnce = true;
                // Mark current file as having #pragma once
                if (!inclusionStack.empty()) {
                    pragmaOnceFiles.insert(inclusionStack.back());
                }
            }
        }
    }
    
    std::string pragmaContent;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Whitespace) {
            if (!pragmaContent.empty()) {
                pragmaContent += " ";
            }
        } else {
            pragmaContent += tok.lexeme;
        }
    }
    
    // Trim trailing whitespace
    while (!pragmaContent.empty() && (pragmaContent.back() == ' ' || pragmaContent.back() == '\t')) {
        pragmaContent.pop_back();
    }
    
    // Emit a diagnostic about the pragma (only if not pragma once, which is handled silently)
    if (!isPragmaOnce) {
        diagnostics.push_back(Diagnostic{
            .message = std::string("#pragma: ") + (pragmaContent.empty() ? "(empty)" : pragmaContent),
            .severity = Diagnostic::Severity::Warning, // Use warning level for visibility
            .span = tokens.empty() ? std::nullopt : std::optional<SourceSpan>(tokens[0].span)
        });
    }
    
    return true; // #pragma does not cause preprocessing to fail
}

} // namespace wvmcc
