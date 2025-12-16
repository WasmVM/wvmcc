#include "Preprocessor.hpp"

#include <fstream>
#include <sstream>

namespace wvmcc {

PreprocessResult Preprocessor::run(const std::string& inputPath) const {
    std::ifstream ifs(inputPath);
    if (!ifs) {
        return PreprocessResult{std::string(), false, std::string("cannot open input: ") + inputPath};
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string phase1 = phase1_normalize(oss.str());
    std::string phase2 = phase2_line_splice(phase1);
    std::string phase3 = phase3_strip_comments(phase2);
    return PreprocessResult{phase3, true, std::string()};
}

// Phase 1 implementation: trigraphs, EOL normalization, final newline
std::string Preprocessor::phase1_normalize(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    // Replace trigraphs
    for (size_t i = 0; i < input.size(); ++i) {
        if (i + 2 < input.size() && input[i] == '?' && input[i + 1] == '?') {
            char third = input[i + 2];
            switch (third) {
                case '=': out.push_back('#'); i += 2; continue;
                case '(': out.push_back('['); i += 2; continue;
                case '/': out.push_back('\\'); i += 2; continue;
                case ')': out.push_back(']'); i += 2; continue;
                case '\'': out.push_back('^'); i += 2; continue;
                case '<': out.push_back('{'); i += 2; continue;
                case '!': out.push_back('|'); i += 2; continue;
                case '>': out.push_back('}'); i += 2; continue;
                case '-': out.push_back('~'); i += 2; continue;
                default: break; // not a trigraph, fall through
            }
        }
        out.push_back(input[i]);
    }

    // Normalize end-of-line: treat CRLF and CR as LF ('\n')
    std::string eol;
    eol.reserve(out.size());
    for (size_t i = 0; i < out.size(); ++i) {
        char c = out[i];
        if (c == '\r') {
            // If CRLF, skip CR and ensure single LF
            if (i + 1 < out.size() && out[i + 1] == '\n') {
                eol.push_back('\n');
                ++i; // skip LF consumption by loop increment
            } else {
                eol.push_back('\n');
            }
        } else {
            eol.push_back(c);
        }
    }

    // Ensure file ends with a newline if not empty and not ending with backslash
    if (!eol.empty()) {
        if (eol.back() != '\n') {
            eol.push_back('\n');
        }
    }

    return eol;
}

// Phase 2 implementation: line splicing of a backslash at end of physical line
// After Phase 1, input uses only '\n' as line ending.
// If a line ends with a single backslash before the newline, remove the backslash and newline.
// Only the final backslash character immediately preceding the newline is eligible.
std::string Preprocessor::phase2_line_splice(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '\n') {
            if (!out.empty() && out.back() == '\\') {
                // splice: remove the trailing backslash and skip this newline
                out.pop_back();
                continue; // do not append '\n'
            }
            out.push_back('\n');
        } else {
            out.push_back(c);
        }
    }

    return out;
}

// Phase 3 implementation: strip comments replacing them with a single space.
// - "/* ... */" comments may span lines; replaced by one space.
// - "// ...\n" comments run to end of line; replaced by nothing but the retained newline.
// - Do not treat sequences inside character or string literals as comments.
std::string Preprocessor::phase3_strip_comments(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    enum class State { Normal, InString, InChar, InBlockComment, InLineComment };
    State st = State::Normal;
    bool escape = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

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
                            // enter block comment, emit single space placeholder
                            st = State::InBlockComment;
                            out.push_back(' ');
                            ++i; // consume '*'
                        } else if (n == '/') {
                            st = State::InLineComment;
                            ++i; // consume second '/'
                            // do not emit anything for the comment content
                        } else {
                            out.push_back(c);
                        }
                    } else {
                        out.push_back(c);
                    }
                    break;
                } else {
                    out.push_back(c);
                }
                break;
            }
            case State::InString: {
                out.push_back(c);
                if (escape) {
                    escape = false;
                } else {
                    if (c == '\\') escape = true;
                    else if (c == '"') st = State::Normal;
                }
                break;
            }
            case State::InChar: {
                out.push_back(c);
                if (escape) {
                    escape = false;
                } else {
                    if (c == '\\') escape = true;
                    else if (c == '\'') st = State::Normal;
                }
                break;
            }
            case State::InBlockComment: {
                if (c == '*' && i + 1 < input.size() && input[i + 1] == '/') {
                    st = State::Normal;
                    ++i; // consume '/'
                }
                // otherwise ignore comment content
                break;
            }
            case State::InLineComment: {
                if (c == '\n') {
                    st = State::Normal;
                    out.push_back('\n'); // retain newline per phase 3 rules
                }
                // else ignore until newline
                break;
            }
        }
    }

    return out;
}

} // namespace wvmcc
