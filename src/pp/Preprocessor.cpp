#include "Preprocessor.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include "Tokenizer.hpp"

namespace wvmcc {

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
            Preprocessor child;
            child.includePaths = includePaths;
            child.inclusionStack = inclusionStack;
            auto childRes = child.run(*resolved);
            diagnostics.insert(diagnostics.end(), child.diagnostics.begin(), child.diagnostics.end());
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
    
    popInclusion();
    
    // Return failure if any errors were encountered during preprocessing
    if (hasErrors) {
        return PreprocessResult{std::vector<PPToken>{}, false, std::string("preprocessing failed with errors")};
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

std::vector<PPToken> Preprocessor::expandMacros(const std::vector<PPToken>& tokens) {
    std::vector<PPToken> result;
    std::unordered_set<std::string> expandedMacros; // Prevent infinite recursion
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];
        
        // Skip expansion for non-identifiers
        if (t.kind != PPTokenKind::Identifier) {
            result.push_back(t);
            continue;
        }
        
        // Skip if already expanded in this invocation
        if (expandedMacros.count(t.lexeme)) {
            result.push_back(t);
            continue;
        }
        
        // Check if this identifier is a defined macro
        auto macro = macroTable.getMacro(t.lexeme);
        if (!macro) {
            result.push_back(t);
            continue;
        }
        
        const Macro* m = *macro;
        
        if (m->isFunction) {
            // Function-like macro: check if next non-whitespace token is '('
            // Note: In C, there must be no space between macro name and (
            // However, we'll be lenient and allow whitespace for now
            size_t j = i + 1;
            
            // Skip whitespace to find '('
            while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) {
                j++;
            }
            
            // Check if we have '(' - if not, don't expand
            if (j >= tokens.size() || tokens[j].kind != PPTokenKind::Punctuator || 
                tokens[j].lexeme != "(") {
                result.push_back(t);
                continue;
            }
            
            // Collect argument tokens between '(' and ')'
            std::vector<std::vector<PPToken>> args;
            size_t argStart = j + 1;
            int parenDepth = 1;
            size_t k = argStart;
            
            while (k < tokens.size() && parenDepth > 0) {
                if (tokens[k].kind == PPTokenKind::Punctuator && tokens[k].lexeme == "(") {
                    parenDepth++;
                } else if (tokens[k].kind == PPTokenKind::Punctuator && tokens[k].lexeme == ")") {
                    parenDepth--;
                    if (parenDepth == 0) {
                        // End of arguments - collect the last argument
                        std::vector<PPToken> lastArg;
                        for (size_t l = argStart; l < k; ++l) {
                            if (tokens[l].kind == PPTokenKind::Punctuator && tokens[l].lexeme == ",") {
                                args.push_back(lastArg);
                                lastArg.clear();
                                argStart = l + 1;
                            } else if (tokens[l].kind != PPTokenKind::Whitespace || !lastArg.empty()) {
                                lastArg.push_back(tokens[l]);
                            }
                        }
                        if (!lastArg.empty() || args.empty()) {
                            args.push_back(lastArg);
                        }
                        break;
                    }
                }
                k++;
            }
            
            // Check argument count matches parameter count
            if (args.size() != m->params.size() && !m->variadic) {
                // Mismatch - don't expand
                result.push_back(t);
                continue;
            }
            
            // Perform substitution: replace parameters in replacement with arguments
            std::vector<PPToken> expanded;
            for (const auto& repl : m->replacement) {
                bool isParam = false;
                for (size_t pIdx = 0; pIdx < m->params.size(); ++pIdx) {
                    if (repl.kind == PPTokenKind::Identifier && repl.lexeme == m->params[pIdx]) {
                        // Replace parameter with argument
                        if (pIdx < args.size()) {
                            for (const auto& arg : args[pIdx]) {
                                expanded.push_back(arg);
                            }
                        }
                        isParam = true;
                        break;
                    }
                }
                if (!isParam) {
                    expanded.push_back(repl);
                }
            }
            
            // Add expanded tokens to result
            for (const auto& exp : expanded) {
                result.push_back(exp);
            }
            
            // Move past the closing )
            i = k;
            expandedMacros.insert(t.lexeme);
            continue;
        }
        
        // Object-like macro expansion: replace with replacement tokens
        expandedMacros.insert(t.lexeme);
        
        // Insert replacement tokens
        for (const auto& repl : m->replacement) {
            result.push_back(repl);
        }
    }
    
    return result;
}

std::optional<int64_t> Preprocessor::evaluateConstantExpression(const std::vector<PPToken>& tokens) {
    // Expand macros first, then evaluate as integer constant expression (C17 6.6)
    auto expanded = expandMacros(tokens);
    
    // Skip leading/trailing whitespace
    size_t i = 0;
    while (i < expanded.size() && expanded[i].kind == PPTokenKind::Whitespace) i++;
    size_t end = expanded.size();
    while (end > i && expanded[end - 1].kind == PPTokenKind::Whitespace) end--;
    
    if (i >= end) {
        diagnostics.push_back(Diagnostic{
            .message = "empty constant expression in #if/#elif",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return std::nullopt;
    }

    // Simple recursive descent parser for integer expressions with operator precedence
    // Helper lambda to skip whitespace
    auto skipWhitespace = [&expanded, &end](size_t& pos) {
        while (pos < end && expanded[pos].kind == PPTokenKind::Whitespace) pos++;
    };
    
    std::function<std::optional<int64_t>(size_t&)> parseExpr;
    std::function<std::optional<int64_t>(size_t&)> parseTernary;
    std::function<std::optional<int64_t>(size_t&)> parseLogicalOr;
    std::function<std::optional<int64_t>(size_t&)> parseLogicalAnd;
    std::function<std::optional<int64_t>(size_t&)> parseBitwiseOr;
    std::function<std::optional<int64_t>(size_t&)> parseBitwiseXor;
    std::function<std::optional<int64_t>(size_t&)> parseBitwiseAnd;
    std::function<std::optional<int64_t>(size_t&)> parseEquality;
    std::function<std::optional<int64_t>(size_t&)> parseRelational;
    std::function<std::optional<int64_t>(size_t&)> parseShift;
    std::function<std::optional<int64_t>(size_t&)> parseAdditive;
    std::function<std::optional<int64_t>(size_t&)> parseMultiplicative;
    std::function<std::optional<int64_t>(size_t&)> parseUnary;
    std::function<std::optional<int64_t>(size_t&)> parsePrimary;

    parseExpr = [&](size_t& pos) -> std::optional<int64_t> {
        return parseTernary(pos);
    };

    parseTernary = [&](size_t& pos) -> std::optional<int64_t> {
        skipWhitespace(pos);
        auto val = parseLogicalOr(pos);
        if (!val) return std::nullopt;
        
        skipWhitespace(pos);
        if (pos < end && expanded[pos].kind == PPTokenKind::Punctuator && expanded[pos].lexeme == "?") {
            pos++;
            auto trueVal = parseExpr(pos);
            if (!trueVal) return std::nullopt;
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != ":") {
                diagnostics.push_back(Diagnostic{
                    .message = "expected ':' in ternary operator",
                    .severity = Diagnostic::Severity::Error,
                    .span = (pos < end) ? std::optional<SourceSpan>(expanded[pos].span) : std::optional<SourceSpan>()
                });
                return std::nullopt;
            }
            pos++;
            auto falseVal = parseTernary(pos);
            if (!falseVal) return std::nullopt;
            return *val ? *trueVal : *falseVal;
        }
        return val;
    };

    parseLogicalOr = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseLogicalAnd(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != "||") break;
            pos++;
            auto right = parseLogicalAnd(pos);
            if (!right) return std::nullopt;
            left = *left || *right;
        }
        return left;
    };

    parseLogicalAnd = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseBitwiseOr(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != "&&") break;
            pos++;
            auto right = parseBitwiseOr(pos);
            if (!right) return std::nullopt;
            left = *left && *right;
        }
        return left;
    };

    parseBitwiseOr = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseBitwiseXor(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != "|" ||
                (pos + 1 < expanded.size() && expanded[pos + 1].lexeme == "|")) break;
            pos++;
            auto right = parseBitwiseXor(pos);
            if (!right) return std::nullopt;
            left = *left | *right;
        }
        return left;
    };

    parseBitwiseXor = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseBitwiseAnd(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != "^") break;
            pos++;
            auto right = parseBitwiseAnd(pos);
            if (!right) return std::nullopt;
            left = *left ^ *right;
        }
        return left;
    };

    parseBitwiseAnd = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseEquality(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != "&" ||
                (pos + 1 < expanded.size() && expanded[pos + 1].lexeme == "&")) break;
            pos++;
            auto right = parseEquality(pos);
            if (!right) return std::nullopt;
            left = *left & *right;
        }
        return left;
    };

    parseEquality = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseRelational(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || 
                (expanded[pos].lexeme != "==" && expanded[pos].lexeme != "!=")) break;
            std::string op = expanded[pos].lexeme;
            pos++;
            auto right = parseRelational(pos);
            if (!right) return std::nullopt;
            left = (op == "==") ? (*left == *right) : (*left != *right);
        }
        return left;
    };

    parseRelational = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseShift(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || 
                (expanded[pos].lexeme != "<" && expanded[pos].lexeme != ">" && 
                 expanded[pos].lexeme != "<=" && expanded[pos].lexeme != ">=")) break;
            std::string op = expanded[pos].lexeme;
            pos++;
            auto right = parseShift(pos);
            if (!right) return std::nullopt;
            if (op == "<") left = *left < *right;
            else if (op == ">") left = *left > *right;
            else if (op == "<=") left = *left <= *right;
            else if (op == ">=") left = *left >= *right;
        }
        return left;
    };

    parseShift = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseAdditive(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || 
                (expanded[pos].lexeme != "<<" && expanded[pos].lexeme != ">>")) break;
            std::string op = expanded[pos].lexeme;
            pos++;
            auto right = parseAdditive(pos);
            if (!right) return std::nullopt;
            left = (op == "<<") ? (*left << *right) : (*left >> *right);
        }
        return left;
    };

    parseAdditive = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseMultiplicative(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || 
                (expanded[pos].lexeme != "+" && expanded[pos].lexeme != "-")) break;
            std::string op = expanded[pos].lexeme;
            pos++;
            auto right = parseMultiplicative(pos);
            if (!right) return std::nullopt;
            left = (op == "+") ? (*left + *right) : (*left - *right);
        }
        return left;
    };

    parseMultiplicative = [&](size_t& pos) -> std::optional<int64_t> {
        auto left = parseUnary(pos);
        if (!left) return std::nullopt;
        while (true) {
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || 
                (expanded[pos].lexeme != "*" && expanded[pos].lexeme != "/" && expanded[pos].lexeme != "%")) break;
            std::string op = expanded[pos].lexeme;
            pos++;
            auto right = parseUnary(pos);
            if (!right) return std::nullopt;
            if (op == "*") left = *left * *right;
            else if (op == "/") {
                if (*right == 0) {
                    diagnostics.push_back(Diagnostic{
                        .message = "division by zero in constant expression",
                        .severity = Diagnostic::Severity::Error,
                        .span = expanded[pos - 1].span
                    });
                    return std::nullopt;
                }
                left = *left / *right;
            } else if (op == "%") {
                if (*right == 0) {
                    diagnostics.push_back(Diagnostic{
                        .message = "modulo by zero in constant expression",
                        .severity = Diagnostic::Severity::Error,
                        .span = expanded[pos - 1].span
                    });
                    return std::nullopt;
                }
                left = *left % *right;
            }
        }
        return left;
    };

    parseUnary = [&](size_t& pos) -> std::optional<int64_t> {
        skipWhitespace(pos);
        if (pos < end && expanded[pos].kind == PPTokenKind::Punctuator && 
            (expanded[pos].lexeme == "!" || expanded[pos].lexeme == "~" || 
             expanded[pos].lexeme == "+" || expanded[pos].lexeme == "-")) {
            std::string op = expanded[pos].lexeme;
            pos++;
            auto val = parseUnary(pos);
            if (!val) return std::nullopt;
            if (op == "!") return !*val;
            else if (op == "~") return ~*val;
            else if (op == "-") return -*val;
            else return *val;
        }
        return parsePrimary(pos);
    };

    parsePrimary = [&](size_t& pos) -> std::optional<int64_t> {
        skipWhitespace(pos);
        if (pos >= end) {
            diagnostics.push_back(Diagnostic{
                .message = "unexpected end of constant expression",
                .severity = Diagnostic::Severity::Error,
                .span = std::nullopt
            });
            return std::nullopt;
        }

        const auto& tok = expanded[pos];

        // Integer constant
        if (tok.kind == PPTokenKind::PPNumber) {
            try {
                int64_t val = std::stoll(tok.lexeme, nullptr, 0);
                pos++;
                return val;
            } catch (...) {
                diagnostics.push_back(Diagnostic{
                    .message = std::string("invalid integer constant: ") + tok.lexeme,
                    .severity = Diagnostic::Severity::Error,
                    .span = tok.span
                });
                return std::nullopt;
            }
        }

        // Character constant
        if (tok.kind == PPTokenKind::CharConst) {
            std::string ch = tok.lexeme;
            if (ch.size() >= 3 && ch.front() == '\'' && ch.back() == '\'') {
                ch = ch.substr(1, ch.size() - 2);
                if (ch.size() == 1) {
                    pos++;
                    return (int64_t)(unsigned char)ch[0];
                }
            }
            diagnostics.push_back(Diagnostic{
                .message = std::string("invalid character constant in expression: ") + tok.lexeme,
                .severity = Diagnostic::Severity::Error,
                .span = tok.span
            });
            return std::nullopt;
        }

        // Parenthesized expression
        if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "(") {
            pos++;
            auto val = parseExpr(pos);
            if (!val) return std::nullopt;
            skipWhitespace(pos);
            if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != ")") {
                diagnostics.push_back(Diagnostic{
                    .message = "expected ')' after expression",
                    .severity = Diagnostic::Severity::Error,
                    .span = (pos < end) ? std::optional<SourceSpan>(expanded[pos].span) : std::optional<SourceSpan>()
                });
                return std::nullopt;
            }
            pos++;
            return val;
        }

        // defined(NAME) operator
        if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "defined") {
            pos++;
            skipWhitespace(pos);
            if (pos >= end) {
                diagnostics.push_back(Diagnostic{
                    .message = "expected '(' or identifier after 'defined'",
                    .severity = Diagnostic::Severity::Error,
                    .span = tok.span
                });
                return std::nullopt;
            }
            bool hasParens = false;
            if (expanded[pos].kind == PPTokenKind::Punctuator && expanded[pos].lexeme == "(") {
                hasParens = true;
                pos++;
                skipWhitespace(pos);
            }
            if (pos >= end || expanded[pos].kind != PPTokenKind::Identifier) {
                diagnostics.push_back(Diagnostic{
                    .message = "expected macro name in 'defined'",
                    .severity = Diagnostic::Severity::Error,
                    .span = (pos < end) ? expanded[pos].span : tok.span
                });
                return std::nullopt;
            }
            std::string macroName = expanded[pos].lexeme;
            pos++;
            if (hasParens) {
                skipWhitespace(pos);
                if (pos >= end || expanded[pos].kind != PPTokenKind::Punctuator || expanded[pos].lexeme != ")") {
                    diagnostics.push_back(Diagnostic{
                        .message = "expected ')' after macro name in 'defined'",
                        .severity = Diagnostic::Severity::Error,
                        .span = (pos < end) ? std::optional<SourceSpan>(expanded[pos].span) : std::optional<SourceSpan>()
                    });
                    return std::nullopt;
                }
                pos++;
            }
            return macroTable.getMacro(macroName) ? 1 : 0;
        }

        // Undefined identifier → 0 (preprocessor convention)
        if (tok.kind == PPTokenKind::Identifier) {
            pos++;
            return 0;
        }

        diagnostics.push_back(Diagnostic{
            .message = std::string("unexpected token in constant expression: ") + tok.lexeme,
            .severity = Diagnostic::Severity::Error,
            .span = tok.span
        });
        return std::nullopt;
    };

    size_t pos = i;
    auto result = parseExpr(pos);
    if (result && pos < end) {
        // Extra tokens after expression
        diagnostics.push_back(Diagnostic{
            .message = "unexpected tokens after constant expression",
            .severity = Diagnostic::Severity::Error,
            .span = expanded[pos].span
        });
        return std::nullopt;
    }
    return result;
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
    bool isDefined = macroTable.getMacro(macroName) != nullptr;
    ConditionalFrame frame;
    frame.seenTrueBranch = isDefined;
    frame.currentlyActive = isDefined;
    frame.inElse = false;
    conditionalStack.push_back(frame);
    return true;
}

bool Preprocessor::handleIfndefDirective(const std::string& macroName) {
    bool isDefined = macroTable.getMacro(macroName) != nullptr;
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

} // namespace wvmcc
