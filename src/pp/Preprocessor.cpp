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
    std::string combined = phase1to3_process(oss.str());
    auto tokens = Tokenizer::tokenize_with_punctuators(combined);
    // Validate tokens for errors (e.g., unterminated string literal)
    for (const auto& t : tokens) {
        if (t.kind == PPTokenKind::StringLiteral) {
            if (t.lexeme.empty() || t.lexeme.back() != '"') {
                std::ostringstream em;
                em << "unterminated string literal at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
                return PreprocessResult{std::vector<PPToken>{}, false, em.str()};
            }
        } else if (t.kind == PPTokenKind::CharConst) {
            if (t.lexeme.empty() || t.lexeme.back() != '\'') {
                std::ostringstream em;
                em << "unterminated character constant at line " << t.span.begin.line
                   << ", column " << t.span.begin.column;
                return PreprocessResult{std::vector<PPToken>{}, false, em.str()};
            }
        }
    }
    // Post-pass: recognize header-name tokens only within #include
    std::vector<PPToken> transformed;
    transformed.reserve(tokens.size());
    for (size_t i=0;i<tokens.size();) {
        const auto& t = tokens[i];
        if (t.kind == PPTokenKind::Punctuator && t.lexeme == "#") {
            size_t j = i + 1;
            while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) j++;
            // Leverage Identifier recognition for 'include'
            if (j < tokens.size() && tokens[j].kind == PPTokenKind::Identifier && tokens[j].lexeme == "include") {
                transformed.push_back(t);
                transformed.push_back(tokens[j]);
                j++;
                while (j < tokens.size() && tokens[j].kind == PPTokenKind::Whitespace) j++;
                if (j < tokens.size()) {
                    if (tokens[j].kind == PPTokenKind::Punctuator && tokens[j].lexeme == "<") {
                        SourcePos begin = tokens[j].span.begin;
                        std::string acc;
                        size_t k = j + 1;
                        bool terminated = false;
                        while (k < tokens.size()) {
                            if (tokens[k].kind == PPTokenKind::Newline) break;
                            if (tokens[k].kind == PPTokenKind::Punctuator && tokens[k].lexeme == ">") { terminated = true; break; }
                            acc += tokens[k].lexeme;
                            k++;
                        }
                        if (terminated) {
                            SourcePos end = tokens[k].span.end;
                            transformed.push_back(PPToken{PPTokenKind::HeaderName, SourceSpan{begin, end}, std::string("<") + acc + ">"});
                            i = k + 1;
                            continue;
                        }
                    } else if (tokens[j].kind == PPTokenKind::StringLiteral) {
                        transformed.push_back(PPToken{PPTokenKind::HeaderName, tokens[j].span, tokens[j].lexeme});
                        i = j + 1;
                        continue;
                    }
                }
            }
        }
        transformed.push_back(t);
        i++;
    }

    return PreprocessResult{std::move(transformed), true, std::string()};
}


// Combined Phase 1–3 single-pass implementation.
// Performs: trigraph replacement, EOL normalization to '\n', ensure final newline,
// line splicing of trailing backslashes, and comment stripping respecting strings/chars.
std::string Preprocessor::phase1to3_process(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    enum class State { Normal, InString, InChar, InBlockComment, InLineComment };
    State st = State::Normal;
    bool escape = false;
    bool comment_space_pending = false; // track placeholder emitted for block comment

    // We will iterate input and build output while applying transformations.
    // Maintain logic to handle CR/CRLF normalization on the fly and splicing.
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        // Phase 1: trigraphs handled before other logic when in Normal state
        if (st == State::Normal && c == '?' && i + 2 < input.size() && input[i + 1] == '?') {
            char t = input[i + 2];
            char repl = 0;
            switch (t) {
                case '=': repl = '#'; break;
                case '(': repl = '['; break;
                case '/': repl = '\\'; break;
                case ')': repl = ']'; break;
                case '\'': repl = '^'; break;
                case '<': repl = '{'; break;
                case '!': repl = '|'; break;
                case '>': repl = '}'; break;
                case '-': repl = '~'; break;
                default: break;
            }
            if (repl) {
                out.push_back(repl);
                i += 2;
                continue;
            }
        }

        // Normalize EOL: treat CRLF/CR as LF
        if (c == '\r') {
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                c = '\n';
                ++i; // skip LF
            } else {
                c = '\n';
            }
        }

        switch (st) {
            case State::Normal: {
                if (c == '"') {
                    st = State::InString;
                    out.push_back(c);
                    escape = false;
                } else if (c == '\'') {
                    st = State::InChar;
                    out.push_back(c);
                    escape = false;
                } else if (c == '/') {
                    if (i + 1 < input.size()) {
                        char n = input[i + 1];
                        if (n == '*') {
                            st = State::InBlockComment;
                            // Emit a single space placeholder only if previous isn't whitespace
                            if (out.empty() || (out.back() != ' ' && out.back() != '\n' && out.back() != '\t' && out.back() != '\r' && out.back() != '\f' && out.back() != '\v')) {
                                out.push_back(' ');
                                comment_space_pending = true;
                            } else {
                                // Previous is whitespace; avoid double spacing
                                comment_space_pending = true;
                            }
                            ++i; // consume '*'
                        } else if (n == '/') {
                            st = State::InLineComment;
                            ++i; // consume second '/'
                        } else {
                            out.push_back(c);
                        }
                    } else {
                        out.push_back(c);
                    }
                } else if (c == '\n') {
                    // Phase 2: splice if last char is backslash
                    if (!out.empty() && out.back() == '\\') {
                        out.pop_back();
                        // skip appending newline (spliced)
                    } else {
                        // Keep newline even after block comment placeholder
                        out.push_back('\n');
                        comment_space_pending = false;
                    }
                } else {
                    out.push_back(c);
                    comment_space_pending = false;
                }
                break;
            }
            case State::InString: {
                // Strings: no comment recognition; maintain escapes
                if (c == '\n') {
                    out.push_back('\n');
                    escape = false; // escapes do not span lines
                } else {
                    out.push_back(c);
                    if (escape) escape = false;
                    else if (c == '\\') escape = true;
                    else if (c == '"') st = State::Normal;
                }
                break;
            }
            case State::InChar: {
                if (c == '\n') {
                    out.push_back('\n');
                    escape = false;
                } else {
                    out.push_back(c);
                    if (escape) escape = false;
                    else if (c == '\\') escape = true;
                    else if (c == '\'') st = State::Normal;
                }
                break;
            }
            case State::InBlockComment: {
                // Look for end of block comment
                if (c == '*' && i + 1 < input.size() && input[i + 1] == '/') {
                    st = State::Normal;
                    ++i; // consume '/'
                    // Keep comment_space_pending true until we see the next non-newline char
                }
                // else ignore content
                break;
            }
            case State::InLineComment: {
                // Ignore until newline; keep newline
                if (c == '\n') {
                    st = State::Normal;
                    out.push_back('\n');
                }
                break;
            }
        }
    }

    // Ensure final newline
    if (!out.empty() && out.back() != '\n') {
        out.push_back('\n');
    }

    return out;
}


} // namespace wvmcc
