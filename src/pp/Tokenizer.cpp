#include "Tokenizer.hpp"
#include <cstring>

namespace wvmcc {

static size_t match_punct(const std::string& s, size_t i) {
    // Fast path: dispatch by first character, checking longest candidates first per starter.
    const size_t n = s.size() - i;
    const char c = s[i];
    switch (c) {
        case '%':
            if (n >= 4 && s[i+1]==':' && s[i+2]=='%' && s[i+3]==':') return 4; // %:%:
            if (n >= 2 && s[i+1]==':') return 2; // %:
            if (n >= 2 && s[i+1]=='>') return 2; // %>
            if (n >= 2 && s[i+1]=='=') return 2; // %=
            return 1;
        case '<':
            if (n >= 3 && s[i+1]=='<' && s[i+2]=='=') return 3; // <<=
            if (n >= 2 && s[i+1]==':') return 2; // <:
            if (n >= 2 && s[i+1]=='%') return 2; // <%
            if (n >= 2 && s[i+1]=='<') return 2; // <<
            if (n >= 2 && s[i+1]=='=') return 2; // <=
            return 1;
        case '>':
            if (n >= 3 && s[i+1]=='>' && s[i+2]=='=') return 3; // >>=
            if (n >= 2 && s[i+1]=='>' ) return 2; // >>
            if (n >= 2 && s[i+1]=='=') return 2; // >=
            return 1;
        case ':':
            if (n >= 2 && s[i+1]=='>') return 2; // :>
            return 1;
        case '#':
            if (n >= 2 && s[i+1]=='#') return 2; // ##
            return 1;
        case '-':
            if (n >= 2 && s[i+1]=='>') return 2; // ->
            if (n >= 2 && s[i+1]=='-') return 2; // --
            if (n >= 2 && s[i+1]=='=') return 2; // -=
            return 1;
        case '+':
            if (n >= 2 && s[i+1]=='+') return 2; // ++
            if (n >= 2 && s[i+1]=='=') return 2; // +=
            return 1;
        case '&':
            if (n >= 2 && s[i+1]=='&') return 2; // &&
            if (n >= 2 && s[i+1]=='=') return 2; // &=
            return 1;
        case '|':
            if (n >= 2 && s[i+1]=='|') return 2; // ||
            if (n >= 2 && s[i+1]=='=') return 2; // |=
            return 1;
        case '*':
            if (n >= 2 && s[i+1]=='=') return 2; // *=
            return 1;
        case '/':
            if (n >= 2 && s[i+1]=='=') return 2; // /=
            return 1;
        case '^':
            if (n >= 2 && s[i+1]=='=') return 2; // ^=
            return 1;
        case '!':
            if (n >= 2 && s[i+1]=='=') return 2; // !=
            return 1;
        case '=':
            if (n >= 2 && s[i+1]=='=') return 2; // ==
            return 1;
        case '.':
            if (n >= 3 && s[i+1]=='.' && s[i+2]=='.') return 3; // ...
            return 1;
        case '[': case ']': case '(': case ')': case '{': case '}':
        case ',': case ';': case '~': case '?':
            return 1;
        default:
            return 0;
    }
}

SourceBuffer::SourceBuffer(const std::string& input) : inputRef(input) {
    charBuf.reserve(64);
    inputEndsWithNewline = (!inputRef.empty() && (inputRef.back() == '\n' || inputRef.back() == '\r'));
}

char SourceBuffer::trigraph_at(std::size_t idx) const {
    if (idx + 2 < inputRef.size() && inputRef[idx] == '?' && inputRef[idx+1] == '?') {
        switch (inputRef[idx+2]) {
            case '=': return '#';
            case '(': return '[';
            case '/': return '\\';
            case ')': return ']';
            case '\'': return '^';
            case '<': return '{';
            case '!': return '|';
            case '>': return '}';
            case '-': return '~';
        }
    }
    return 0;
}

void SourceBuffer::fill_buffer() {
    while (charBuf.empty() && rawIdx < inputRef.size()) {
        char c = inputRef[rawIdx];
        // EOL normalize
        if (c == '\r') {
            if (rawIdx + 1 < inputRef.size() && inputRef[rawIdx+1] == '\n') { rawIdx += 2; charBuf.push_back('\n'); }
            else { ++rawIdx; charBuf.push_back('\n'); }
            break;
        }
        if (st == State::Normal) {
            // trigraphs
            char tr = trigraph_at(rawIdx);
            if (tr) { rawIdx += 3; charBuf.push_back(tr); break; }
            // enter string/char literals
            if (c == '"') { charBuf.push_back(c); ++rawIdx; st = State::InString; esc = false; break; }
            if (c == '\'') { charBuf.push_back(c); ++rawIdx; st = State::InChar; esc = false; break; }
            // comments
            if (c == '/' && rawIdx + 1 < inputRef.size()) {
                char n = inputRef[rawIdx+1];
                if (n == '*') { st = State::InBlockComment; rawIdx += 2; continue; }
                if (n == '/') { st = State::InLineComment; rawIdx += 2; continue; }
            }
            // line splicing
            if (c == '\\') {
                if (rawIdx + 1 < inputRef.size()) {
                    if (inputRef[rawIdx+1] == '\n') { rawIdx += 2; continue; }
                    if (inputRef[rawIdx+1] == '\r') {
                        if (rawIdx + 2 < inputRef.size() && inputRef[rawIdx+2] == '\n') { rawIdx += 3; continue; }
                        rawIdx += 2; continue;
                    }
                }
            }
            // normal emission
            charBuf.push_back(c);
            ++rawIdx;
            break;
        } else if (st == State::InString) {
            charBuf.push_back(c);
            ++rawIdx;
            if (!esc) { if (c == '\\') esc = true; else if (c == '"') st = State::Normal; }
            else { esc = false; }
            break;
        } else if (st == State::InChar) {
            charBuf.push_back(c);
            ++rawIdx;
            if (!esc) { if (c == '\\') esc = true; else if (c == '\'') st = State::Normal; }
            else { esc = false; }
            break;
        } else if (st == State::InBlockComment) {
            if (c == '*' && rawIdx + 1 < inputRef.size() && inputRef[rawIdx+1] == '/') {
                rawIdx += 2;
                st = State::Normal;
                // Insert a single space to separate tokens around removed block comment
                if (!lastEmittedWhitespace) {
                    charBuf.push_back(' ');
                }
                break;
            }
            ++rawIdx; continue;
        } else if (st == State::InLineComment) {
            if (c == '\n') { ++rawIdx; charBuf.push_back('\n'); st = State::Normal; break; }
            ++rawIdx; continue;
        }
    }
    // ensure final newline only if original input didn't end with newline
    if (rawIdx >= inputRef.size() && charBuf.empty()) {
        if (!inputEndsWithNewline) {
            charBuf.push_back('\n');
            inputEndsWithNewline = true;
        }
    }
}

bool SourceBuffer::next_char(char& outCh) {
    if (charBuf.empty()) fill_buffer();
    if (charBuf.empty()) return false;
    outCh = charBuf.front();
    charBuf.erase(charBuf.begin());
    // advance pos for emitted character
    if (outCh == '\n') { ++pos.line; pos.column = 1; } else { ++pos.column; }
    ++pos.offset;
    lastEmittedWhitespace = (outCh == '\n' || outCh == ' ' || outCh == '\t' || outCh == '\v' || outCh == '\f');
    return true;
}

void SourceBuffer::ensure_stream(std::string& stream, std::size_t upto) {
    while (stream.size() <= upto) {
        char ch; if (!next_char(ch)) break; stream.push_back(ch);
    }
}

bool Tokenizer::is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool Tokenizer::is_nondigit(char c) {
    return (c == '_') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Tokenizer::is_space(char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

bool Tokenizer::is_hex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Tokenizer::is_oct(char c) {
    return c >= '0' && c <= '7';
}

Tokenizer::Tokenizer(const std::string& input) : input(input), feeder(input) {}

void Tokenizer::emit(PPTokenKind kind, const std::string& lexeme, SourcePos begin, SourcePos end) {
    tokens.push_back(PPToken{kind, SourceSpan{begin, end}, lexeme});
}

bool Tokenizer::try_ucn(size_t idx, size_t& consumed) {
    consumed = 0;
    if (idx >= stream.size()) feeder.ensure_stream(stream, idx);
    if (idx >= stream.size() || stream[idx] != '\\') return false;
    feeder.ensure_stream(stream, idx + 1);
    if (idx + 1 < stream.size() && stream[idx+1] == 'u') {
        // \\uXXXX (exactly 4 hex digits)
        feeder.ensure_stream(stream, idx + 5);
        if (idx + 6 <= stream.size()) {
            for (size_t k = idx+2; k < idx+6; ++k) {
                char ch = stream[k];
                if (!Tokenizer::is_hex(ch)) return false;
            }
            consumed = 6; return true;
        }
        return false;
    }
    if (idx + 1 < stream.size() && stream[idx+1] == 'U') {
        // \\UXXXXXXXX (exactly 8 hex digits)
        feeder.ensure_stream(stream, idx + 9);
        if (idx + 10 <= stream.size()) {
            for (size_t k = idx+2; k < idx+10; ++k) {
                char ch = stream[k];
                if (!Tokenizer::is_hex(ch)) return false;
            }
            consumed = 10; return true;
        }
        return false;
    }
    return false;
}

bool Tokenizer::starts_char(size_t idx, size_t& prefixLen) {
    prefixLen = 0;
    if (idx >= stream.size()) return false;
    if (stream[idx] == '\'') { prefixLen = 0; return true; }
    if ((stream[idx] == 'L' || stream[idx] == 'u' || stream[idx] == 'U') && idx + 1 < stream.size() && stream[idx + 1] == '\'') { prefixLen = 1; return true; }
    return false;
}

bool Tokenizer::starts_string(size_t idx, size_t& prefixLen) {
    prefixLen = 0;
    if (idx >= stream.size()) return false;
    if (stream[idx] == '"') { prefixLen = 0; return true; }
    if (stream[idx] == 'u') {
        if (idx + 2 < stream.size() && stream[idx + 1] == '8' && stream[idx + 2] == '"') { prefixLen = 2; return true; }
        if (idx + 1 < stream.size() && stream[idx + 1] == '"') { prefixLen = 1; return true; }
    } else if (stream[idx] == 'U' || stream[idx] == 'L') {
        if (idx + 1 < stream.size() && stream[idx + 1] == '"') { prefixLen = 1; return true; }
    }
    return false;
}

std::vector<PPToken> Tokenizer::tokenize() {
    // Single-pass: tokenize directly over input without a separate preprocess call
    tokens.clear();
    tokens.reserve(input.size() / 2);

    stream.clear();
    stream.reserve(input.size());
    streamPos = 0;
    while (true) {
        feeder.ensure_stream(stream, streamPos);
        if (streamPos >= stream.size()) break;
        char c = stream[streamPos];
        SourcePos begin = feeder.position();

        // Preprocessing number (PPNumber)
        feeder.ensure_stream(stream, streamPos + 1);
        if (c == '.' ? (streamPos + 1 < stream.size() && is_digit(stream[streamPos + 1])) : is_digit(c)) {
            size_t j = streamPos;
            // if started with '.', consume it
            if (stream[j] == '.') { ++j; }
            // consume first run character (digit)
            if (j < stream.size() && is_digit(stream[j])) { ++j; }
            // main loop: digits, identifier-nondigit, exponent markers with optional sign, and trailing dot
            while (true) {
                if (j >= stream.size()) feeder.ensure_stream(stream, j);
                if (j >= stream.size()) break;
                char d = stream[j];
                if (d=='e'||d=='E'||d=='p'||d=='P') {
                    // Consume exponent marker
                    ++j;
                    // Optional sign
                    if (j >= stream.size()) feeder.ensure_stream(stream, j);
                    if (j < stream.size() && (stream[j] == '+' || stream[j] == '-')) { ++j; }
                    // Consume subsequent digits (if any) as part of pp-number
                    while (true) {
                        if (j >= stream.size()) feeder.ensure_stream(stream, j);
                        if (j < stream.size() && (stream[j] >= '0' && stream[j] <= '9')) { ++j; }
                        else break;
                    }
                    continue;
                }
                if (is_digit(d) || is_nondigit(d)) { ++j; continue; }
                if (d == '.') { ++j; continue; }
                break;
            }
            std::string lex = stream.substr(streamPos, j - streamPos);
            emit(PPTokenKind::PPNumber, lex, begin, feeder.position());
            streamPos = j;
            continue;
        }

        // Character constant (with optional encoding prefix L/u/U)
        size_t charPrefixLen = 0;
        // Ensure lookahead for char prefix detection
        feeder.ensure_stream(stream, streamPos + 1);
        if (starts_char(streamPos, charPrefixLen)) {
            size_t j = streamPos;
            for (size_t k = 0; k < charPrefixLen; ++k) { ++j; }
            // opening '
            if (j < stream.size() && stream[j] == '\'') { ++j; }
            bool escaped = false;
            while (true) {
                if (j >= stream.size()) feeder.ensure_stream(stream, j);
                if (j >= stream.size()) break;
                char d = stream[j];
                if (!escaped) {
                    if (d == '\\') {
                        ++j; escaped = true; continue;
                    }
                    if (d == '\'') { ++j; break; }
                    if (d == '\n') { break; }
                    ++j;
                } else {
                    // after backslash, handle escape classes
                    if (d == 'x') {
                        ++j;
                        // one or more hex digits
                        while (j < stream.size() && is_hex(stream[j])) { ++j; }
                        escaped = false;
                        continue;
                    }
                    if (d == 'u') {
                        ++j;
                        // exactly 4 hex digits if available
                        for (int k=0;k<4 && j<stream.size() && is_hex(stream[j]);++k){ ++j; }
                        escaped = false; continue;
                    }
                    if (d == 'U') {
                        ++j;
                        // up to 8 hex digits
                        for (int k=0;k<8 && j<stream.size() && is_hex(stream[j]);++k){ ++j; }
                        escaped = false; continue;
                    }
                    // octal: up to 3 oct digits
                    if (is_oct(d)) {
                        ++j;
                        for (int k=1;k<3 && j<stream.size() && is_oct(stream[j]);++k){ ++j; }
                        escaped = false; continue;
                    }
                    // simple escape or other single char after backslash
                    ++j; escaped = false;
                }
            }
            std::string lex = stream.substr(streamPos, j - streamPos);
            emit(PPTokenKind::CharConst, lex, begin, feeder.position());
            streamPos = j;
            continue;
        }

        // String literal (with optional encoding prefix u8/u/U/L)
        size_t prefixLen = 0;
        // Ensure lookahead for string prefix detection (u8/u/U/L)
        feeder.ensure_stream(stream, streamPos + 2);
        if (starts_string(streamPos, prefixLen)) {
            size_t j = streamPos;
            // Emit full literal from start of prefix to ending quote
            // Advance through prefix and opening quote updating pos
            for (size_t k = 0; k < prefixLen; ++k) { ++j; }
            // opening quote
            if (j < stream.size() && stream[j] == '"') { ++j; }
            bool escaped = false;
            while (true) {
                if (j >= stream.size()) feeder.ensure_stream(stream, j);
                if (j >= stream.size()) break;
                char d = stream[j];
                if (!escaped) {
                    if (d == '\\') {
                        ++j; escaped = true; continue;
                    }
                    if (d == '"') {
                        // include closing quote
                        ++j; break;
                    }
                    if (d == '\n') {
                        // Invalid: newline terminates string literal (do not consume) per C spec
                        break;
                    }
                    ++j; // normal char
                } else {
                    // escaped character
                    ++j; escaped = false;
                }
            }
            std::string lex = stream.substr(streamPos, j - streamPos);
            emit(PPTokenKind::StringLiteral, lex, begin, feeder.position());
            streamPos = j;
            continue;
        }

        if (c == '\n') { emit(PPTokenKind::Newline, "\n", begin, feeder.position()); ++streamPos; continue; }

        if (is_space(c)) {
            std::string lex;
            do { lex.push_back(c); ++streamPos; feeder.ensure_stream(stream, streamPos); if (streamPos>=stream.size()) break; c = stream[streamPos]; } while (is_space(c));
            emit(PPTokenKind::Whitespace, lex, begin, feeder.position());
            continue;
        }
        // Identifier: starts with nondigit or universal-character-name
        // Ensure lookahead so UCN detection sees the 'u'/'U' after '\\'
        feeder.ensure_stream(stream, streamPos + 1);
        if (is_nondigit(c) || (c=='\\' && (streamPos+1<stream.size()) && (stream[streamPos+1]=='u' || stream[streamPos+1]=='U'))) {
            size_t j = streamPos;
            bool validStart = false;
            // consume first char or require valid UCN
            if (is_nondigit(stream[j])) { 
                if (j + 1 > stream.size()) feeder.ensure_stream(stream, j + 1);
                ++j; 
                validStart = true;
            } else {
                size_t u = 0; if (try_ucn(j, u)) { j += u; validStart = true; }
            }
            if (validStart) {
                // consume subsequent identifier chars: alnum or underscores or ucn
                while (true) {
                    if (j >= stream.size()) feeder.ensure_stream(stream, j);
                    if (j >= stream.size()) break;
                    if (is_nondigit(stream[j]) || is_digit(stream[j])) { ++j; continue; }
                    size_t u = 0; if (try_ucn(j, u)) { j += u; continue; }
                    break;
                }
                if (j > streamPos) {
                    std::string lex = stream.substr(streamPos, j - streamPos);
                    emit(PPTokenKind::Identifier, lex, begin, feeder.position());
                    streamPos = j; continue;
                }
            }
        }

        size_t plen = 0;
        if (streamPos < stream.size()) {
            // Ensure enough lookahead for longest punctuators (e.g., "%:%:" is 4 chars)
            if (stream.size() - streamPos < 4) feeder.ensure_stream(stream, streamPos + 3);
            plen = match_punct(stream, streamPos);
        }
        if (plen > 0) {
            std::string lex = stream.substr(streamPos, plen);
            streamPos += plen;
            emit(PPTokenKind::Punctuator, lex, begin, feeder.position());
            continue;
        }

        // Fallback: single Other character
        std::string lex(1, c);
        ++streamPos;
        emit(PPTokenKind::Other, lex, begin, feeder.position());
    }

    return tokens;
}

} // namespace wvmcc
