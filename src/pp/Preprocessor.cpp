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
    // TODO: Execute include in Phase 2 using currentDir and includePaths.
    // Skip spaces (do not emit) between directive and header name
    while (auto w = tokenizer.peek()) {
        if (w->kind == PPTokenKind::Whitespace) { tokenizer.next(); }
        else break;
    }
    auto h = tokenizer.peek();
    if (h && h->kind == PPTokenKind::Punctuator && h->lexeme == "<") {
        SourcePos begin = h->span.begin;
        // consume '<' but do not emit it; we'll emit a single HeaderName token
        tokenizer.next();
        std::string acc;
        SourcePos end = begin;
        bool terminated = false;
        while (auto x = tokenizer.peek()) {
            if (x->kind == PPTokenKind::Newline) break;
            if (x->kind == PPTokenKind::Punctuator && x->lexeme == ">") {
                end = x->span.end;
                tokenizer.next();
                // Resolution: try -I search paths for angle form
                if (auto resolved = resolveInclude(acc, /*isAngle=*/true, currentDir)) {
                    includeQueue.push_back(*resolved);
                    // Check for cyclic include
                    std::string canonicalResolved = std::filesystem::weakly_canonical(
                        std::filesystem::absolute(*resolved)).string();
                    if (isInInclusionStack(canonicalResolved)) {
                        std::cerr << "error: cyclic include detected: " << canonicalResolved << "\n";
                        diagnostics.push_back(Diagnostic{
                            .message = std::string("cyclic include detected: ") + canonicalResolved,
                            .severity = Diagnostic::Severity::Error,
                            .span = SourceSpan{begin, end}
                        });
                        return false;  // Fail on cycle
                    } else {
                        // Execute: preprocess included file (supports nested includes) and inline its tokens
                        Preprocessor child;
                        child.includePaths = includePaths;
                        child.inclusionStack = inclusionStack;  // Share the inclusion stack
                        auto childRes = child.run(*resolved);
                        diagnostics.insert(diagnostics.end(), child.diagnostics.begin(), child.diagnostics.end());
                        if (childRes.success) {
                            for (const auto& tk : childRes.tokens) out.push_back(tk);
                            return true;
                        } else {
                            std::cerr << "error: failed to read include file '" << *resolved << "' for <" << acc << ">\n";
                            diagnostics.push_back(Diagnostic{
                                .message = std::string("failed to read include file '") + *resolved + "' for <" + acc + ">",
                                .severity = Diagnostic::Severity::Error,
                                .span = SourceSpan{begin, end}
                            });
                            return false;  // Propagate child failure
                        }
                    }
                } else {
                    std::cerr << "error: include file not found for <" << acc << "> (searched -I paths)\n";
                    diagnostics.push_back(Diagnostic{
                        .message = std::string("include file not found for <") + acc + "> (searched -I paths)",
                        .severity = Diagnostic::Severity::Error,
                        .span = SourceSpan{begin, end}
                    });
                }
                // Fallback: do not emit HeaderName when execution fails; rely on diagnostics
                terminated = true;
                break;
            }
            acc += x->lexeme;
            // consume inner tokens without emitting them
            tokenizer.next();
        }
        // If not terminated, leave as-is (no header-name token)
        (void)terminated;
    } else if (h && h->kind == PPTokenKind::StringLiteral) {
        // Strip quotes and resolve using currentDir then -I paths
        std::string quoted = h->lexeme;
        std::string header;
        if (quoted.size() >= 2 && quoted.front()=='"' && quoted.back()=='"') {
            header = quoted.substr(1, quoted.size()-2);
        } else {
            header = quoted;
        }
        if (auto resolved = resolveInclude(header, /*isAngle=*/false, currentDir)) {
            includeQueue.push_back(*resolved);
            // Check for cyclic include
            std::string canonicalResolved = std::filesystem::weakly_canonical(
                std::filesystem::absolute(*resolved)).string();
            if (isInInclusionStack(canonicalResolved)) {
                std::cerr << "error: cyclic include detected: " << canonicalResolved << "\n";
                diagnostics.push_back(Diagnostic{
                    .message = std::string("cyclic include detected: ") + canonicalResolved,
                    .severity = Diagnostic::Severity::Error,
                    .span = h->span
                });
                tokenizer.next();
                return false;  // Fail on cycle
            } else {
                // Execute: preprocess included file (supports nested includes) and inline its tokens
                Preprocessor child;
                child.includePaths = includePaths;
                child.inclusionStack = inclusionStack;  // Share the inclusion stack
                auto childRes = child.run(*resolved);
                diagnostics.insert(diagnostics.end(), child.diagnostics.begin(), child.diagnostics.end());
                if (childRes.success) {
                    for (const auto& tk : childRes.tokens) out.push_back(tk);
                    tokenizer.next();
                    return true;
                } else {
                    std::cerr << "error: failed to read include file '" << *resolved << "' for \"" << header << "\"\n";
                    diagnostics.push_back(Diagnostic{
                        .message = std::string("failed to read include file '") + *resolved + "' for \"" + header + "\"",
                        .severity = Diagnostic::Severity::Error,
                        .span = h->span
                    });
                    tokenizer.next();
                    return false;  // Propagate child failure
                }
            }
        } else {
            std::cerr << "error: include file not found for \"" << header << "\" (searched current dir and -I paths)\n";
            diagnostics.push_back(Diagnostic{
                .message = std::string("include file not found for \"") + header + "\" (Searched current dir and -I paths)",
                .severity = Diagnostic::Severity::Error,
                .span = h->span
            });
        }
        // Fallback: do not emit HeaderName when execution fails; rely on diagnostics
        tokenizer.next();
    }
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
            out.push_back(t);
            atLineStart = true;
            continue;
        }
        if (atLineStart) {
            if (t.kind == PPTokenKind::Whitespace) {
                out.push_back(t);
                // still at line start until a non-whitespace token
                continue;
            }
            if (t.kind == PPTokenKind::Punctuator && t.lexeme == "#") {
                // Parse directive keyword
                out.push_back(t);
                // Skip spaces (do not emit) between '#' and directive keyword
                while (auto p = tokenizer.peek()) {
                    if (p->kind == PPTokenKind::Whitespace) { tokenizer.next(); }
                    else break;
                }
                auto dir = tokenizer.peek();
                if (dir && dir->kind == PPTokenKind::Identifier) {
                    // Push directive keyword
                    out.push_back(*tokenizer.next());
                    // Special handling for include header-name recognition
                    if (dir->lexeme == "include") {
                        // For executed includes, do not emit '#' or 'include' tokens
                        bool executed = handleIncludeDirective(tokenizer, out, currentDir);
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
                            if (!handleDefineDirective(tokenizer)) {
                                hasErrors = true;
                            }
                        } else if (dir->lexeme == "undef") {
                            if (!handleUndefDirective(tokenizer)) {
                                hasErrors = true;
                            }
                        } else {
                            // Other directives: consume until newline but do not execute; emit as-is
                            // TODO: Implement conditional stack handling (#if/#ifdef/#ifndef/#elif/#else/#endif)
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
            out.push_back(t);
            atLineStart = false;
            continue;
        }
        // Normal token outside of line start
        out.push_back(t);
    }

    if (!endsWithNewline) {
        diagnostics.push_back(Diagnostic{
            .message = std::string("input file '") + inputPath + "' does not end with a newline",
            .severity = Diagnostic::Severity::Warning,
            .span = std::nullopt
        });
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

} // namespace wvmcc
