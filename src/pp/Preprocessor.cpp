#include "Preprocessor.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
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
                    // Execute: stream tokens from included file inline
                    std::ifstream hfs(*resolved);
                    if (hfs) {
                        Tokenizer htok(hfs);
                        htok.reset();
                        while (auto itok = htok.next()) { out.push_back(*itok); }
                        return true;
                    } else {
                        std::cerr << "error: failed to read include file '" << *resolved << "' for <" << acc << ">\n";
                        diagnostics.push_back(Diagnostic{
                            .message = std::string("failed to read include file '") + *resolved + "' for <" + acc + ">",
                            .severity = Diagnostic::Severity::Error,
                            .span = SourceSpan{begin, end}
                        });
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
            // Execute: stream tokens from included file inline
            std::ifstream hfs(*resolved);
            if (hfs) {
                Tokenizer htok(hfs);
                htok.reset();
                while (auto itok = htok.next()) { out.push_back(*itok); }
                tokenizer.next();
                return true;
            } else {
                std::cerr << "error: failed to read include file '" << *resolved << "' for \"" << header << "\"\n";
                diagnostics.push_back(Diagnostic{
                    .message = std::string("failed to read include file '") + *resolved + "' for \"" + header + "\"",
                    .severity = Diagnostic::Severity::Error,
                    .span = h->span
                });
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

PreprocessResult Preprocessor::run(const std::string& inputPath) {
    std::ifstream ifs(inputPath);
    if (!ifs) {
        return PreprocessResult{std::vector<wvmcc::PPToken>{}, false, std::string("cannot open input: ") + inputPath};
    }
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
    while (auto tokOpt = tokenizer.next()) {
        PPToken t = *tokOpt;
        // Basic validation for literals on the fly
        if (t.kind == PPTokenKind::StringLiteral) {
            if (t.lexeme.empty() || t.lexeme.back() != '"') {
                std::ostringstream em;
                em << "unterminated string literal at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
                return PreprocessResult{std::vector<PPToken>{}, false, em.str()};
            }
        } else if (t.kind == PPTokenKind::CharConst) {
            if (t.lexeme.empty() || t.lexeme.back() != '\'' ) {
                std::ostringstream em;
                em << "unterminated character constant at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
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
                            // Not executed: emit '#' and 'include' as part of directive line
                            out.push_back(t);
                            out.push_back(*dir);
                        }
                    } else {
                        // Other directives: consume until newline but do not execute; emit as-is
                        // TODO: Parse and store directive arguments for later execution
                        // TODO: Implement execution for object-like macros (#define/#undef)
                        // TODO: Implement conditional stack handling (#if/#ifdef/#ifndef/#elif/#else/#endif)
                        while (auto a = tokenizer.peek()) {
                            if (a->kind == PPTokenKind::Newline) break;
                            out.push_back(*tokenizer.next());
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
    return PreprocessResult{std::move(out), true, std::string()};
}

} // namespace wvmcc
