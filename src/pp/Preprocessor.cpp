#include "Preprocessor.hpp"

#include <fstream>
#include <sstream>
#include "Tokenizer.hpp"

namespace wvmcc {

PreprocessResult Preprocessor::run(const std::string& inputPath) const {
    std::ifstream ifs(inputPath);
    if (!ifs) {
        return PreprocessResult{std::vector<wvmcc::PPToken>{}, false, std::string("cannot open input: ") + inputPath};
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    // Single-pass streaming: consume tokens, parse directives (no execution yet), and recognize header-name in #include
    Tokenizer tokenizer(oss.str());
    std::vector<PPToken> out;
    out.reserve(oss.str().size() / 2);
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
                        // TODO: Execute include: resolve header, push new token source, and continue streaming
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
                                    out.push_back(PPToken{PPTokenKind::HeaderName, SourceSpan{begin, end}, std::string("<") + acc + ">"});
                                    terminated = true;
                                    break;
                                }
                                acc += x->lexeme;
                                // consume inner tokens without emitting them
                                tokenizer.next();
                            }
                            // If not terminated, leave as-is (no header-name token)
                        } else if (h && h->kind == PPTokenKind::StringLiteral) {
                            out.push_back(PPToken{PPTokenKind::HeaderName, h->span, h->lexeme});
                            tokenizer.next();
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

    return PreprocessResult{std::move(out), true, std::string()};
}

} // namespace wvmcc
