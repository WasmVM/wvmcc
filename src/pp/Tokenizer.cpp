#include "Tokenizer.hpp"
#include <cstring>

namespace wvmcc {

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

// Greedy punctuator recognition set (includes digraphs and multi-char operators)
static const char* const PUNCTS[] = {
    "<:",":>","<%","%>","%:","%:%:",
    "->","++","--","<<",">>","<=",">=","==","!=","&&","||","...",
    "*=","/=","%=","+=","-=","<<=",">>=","&=","^=","|=",
    "[", "]", "(", ")", "{", "}", ".", ",", ";",
    "&","*","+","-","~","!","/","%","<",">","^","|","?",":","=","#","##"
};

static bool is_punct_start(char c) {
    // Fast path: characters that can start punctuators
    switch (c) {
        case '[': case ']': case '(': case ')': case '{': case '}': case '.': case ',': case ';':
        case '&': case '*': case '+': case '-': case '~': case '!': case '/': case '%':
        case '<': case '>': case '^': case '|': case '?': case ':': case '=': case '#':
            return true;
        default: return false;
    }
}

static size_t match_punct(const std::string& s, size_t i) {
    if (!is_punct_start(s[i])) return 0;
    // Try longest matches first
    size_t maxLen = 0;
    for (const char* p : PUNCTS) {
        size_t len = std::strlen(p);
        if (i + len <= s.size() && std::strncmp(&s[i], p, len) == 0) {
            if (len > maxLen) maxLen = len;
        }
    }
    return maxLen;
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

std::vector<PPToken> Tokenizer::tokenize(const std::string& input) {
    // Single-pass: tokenize directly over input without a separate preprocess call
    const std::string& inputRef = input;
    std::vector<PPToken> out;
    out.reserve(inputRef.size() / 2);

    SourcePos pos{0, 1, 1, 0};
    auto advance_pos = [&](char ch) {
        if (ch == '\n') {
            ++pos.line;
            pos.column = 1;
        } else {
            ++pos.column;
        }
        ++pos.offset;
    };
    auto emit = [&](PPTokenKind kind, const std::string& lexeme, SourcePos begin, SourcePos end) {
        out.push_back(PPToken{kind, SourceSpan{begin, end}, lexeme});
    };

    // Use BufferedFeeder to build a dynamic stream for tokenization
    SourceBuffer feeder(inputRef);
    std::string stream;
    stream.reserve(inputRef.size());
    auto ensure_stream = [&](size_t upto) { feeder.ensure_stream(stream, upto); };

    size_t i = 0;
    while (true) {
        ensure_stream(i);
        if (i >= stream.size()) break;
        char c = stream[i];
        SourcePos begin = feeder.position();

        // Preprocessing number (PPNumber)
        auto is_digit = [](char ch){ return ch>='0' && ch<='9'; };
        auto is_nondigit = [](char ch){ return (ch=='_') || (ch>='a'&&ch<='z') || (ch>='A'&&ch<='Z'); };
        // Ensure lookahead for pp-number start
        ensure_stream(i + 1);
        if (c == '.' ? (i + 1 < stream.size() && is_digit(stream[i+1])) : is_digit(c)) {
            size_t j = i;
            bool afterExpMarker = false;
            // if started with '.', consume it
            if (stream[j] == '.') { ++j; }
            // consume first run character (digit)
            if (j < stream.size() && is_digit(stream[j])) { ++j; }
            // main loop: digits, identifier-nondigit, exponent markers with optional sign, and trailing dot
            while (true) {
                if (j >= stream.size()) ensure_stream(j);
                if (j >= stream.size()) break;
                char d = stream[j];
                if (d=='e'||d=='E'||d=='p'||d=='P') {
                    // Consume exponent marker
                    ++j;
                    // Optional sign
                    if (j >= stream.size()) ensure_stream(j);
                    if (j < stream.size() && (stream[j] == '+' || stream[j] == '-')) { ++j; }
                    // Consume subsequent digits (if any) as part of pp-number
                    while (true) {
                        if (j >= stream.size()) ensure_stream(j);
                        if (j < stream.size() && (stream[j] >= '0' && stream[j] <= '9')) { ++j; }
                        else break;
                    }
                    continue;
                }
                if (is_digit(d) || is_nondigit(d)) { ++j; continue; }
                if (d == '.') { ++j; continue; }
                break;
            }
            std::string lex = stream.substr(i, j - i);
            emit(PPTokenKind::PPNumber, lex, begin, feeder.position());
            i = j;
            continue;
        }

        // Character constant (with optional encoding prefix L/u/U)
        auto starts_char = [&](size_t idx, size_t& prefixLen) -> bool {
            prefixLen = 0;
            if (idx >= stream.size()) return false;
            if (stream[idx] == '\'') { prefixLen = 0; return true; }
            if ((stream[idx] == 'L' || stream[idx] == 'u' || stream[idx] == 'U') && idx + 1 < stream.size() && stream[idx + 1] == '\'') { prefixLen = 1; return true; }
            return false;
        };
        size_t charPrefixLen = 0;
        // Ensure lookahead for char prefix detection
        ensure_stream(i + 1);
        if (starts_char(i, charPrefixLen)) {
            size_t j = i;
            for (size_t k = 0; k < charPrefixLen; ++k) { ++j; }
            // opening '
            if (j < stream.size() && stream[j] == '\'') { ++j; }
            bool escaped = false;
            auto is_hex = [](char ch){ return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F'); };
            auto is_oct = [](char ch){ return ch>='0'&&ch<='7'; };
            while (true) {
                if (j >= stream.size()) ensure_stream(j);
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
            std::string lex = stream.substr(i, j - i);
            emit(PPTokenKind::CharConst, lex, begin, feeder.position());
            i = j;
            continue;
        }

        // String literal (with optional encoding prefix u8/u/U/L)
        auto starts_string = [&](size_t idx, size_t& prefixLen) -> bool {
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
        };
        size_t prefixLen = 0;
        // Ensure lookahead for string prefix detection (u8/u/U/L)
        ensure_stream(i + 2);
        if (starts_string(i, prefixLen)) {
            size_t j = i;
            // Emit full literal from start of prefix to ending quote
            // Advance through prefix and opening quote updating pos
            for (size_t k = 0; k < prefixLen; ++k) { ++j; }
            // opening quote
            if (j < stream.size() && stream[j] == '"') { ++j; }
            bool escaped = false;
            while (true) {
                if (j >= stream.size()) ensure_stream(j);
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
            std::string lex = stream.substr(i, j - i);
            emit(PPTokenKind::StringLiteral, lex, begin, feeder.position());
            i = j;
            continue;
        }

        if (c == '\n') { emit(PPTokenKind::Newline, "\n", begin, feeder.position()); ++i; continue; }

        if (is_space(c)) {
            std::string lex;
            do { lex.push_back(c); ++i; ensure_stream(i); if (i>=stream.size()) break; c = stream[i]; } while (is_space(c));
            emit(PPTokenKind::Whitespace, lex, begin, feeder.position());
            continue;
        }

        // Identifier: starts with nondigit or universal-character-name
        auto is_alpha = [](char ch){ return (ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||ch=='_'; };
        auto is_alnum_us = [&](char ch){ return is_alpha(ch) || (ch>='0'&&ch<='9'); };
        auto try_ucn = [&](size_t idx, size_t& consumed){
            consumed = 0;
            if (idx >= stream.size()) ensure_stream(idx);
            if (idx >= stream.size() || stream[idx] != '\\') return false;
            ensure_stream(idx + 1);
            if (idx + 1 < stream.size() && stream[idx+1] == 'u') {
                // \\uXXXX (exactly 4 hex digits)
                ensure_stream(idx + 5);
                if (idx + 6 <= stream.size()) {
                    for (size_t k = idx+2; k < idx+6; ++k) {
                        char ch = stream[k];
                        bool hex = (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F');
                        if (!hex) return false;
                    }
                    consumed = 6; return true;
                }
                return false;
            }
            if (idx + 1 < stream.size() && stream[idx+1] == 'U') {
                // \\UXXXXXXXX (exactly 8 hex digits)
                ensure_stream(idx + 9);
                if (idx + 10 <= stream.size()) {
                    for (size_t k = idx+2; k < idx+10; ++k) {
                        char ch = stream[k];
                        bool hex = (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F');
                        if (!hex) return false;
                    }
                    consumed = 10; return true;
                }
                return false;
            }
            return false;
        };
        // Ensure lookahead so UCN detection sees the 'u'/'U' after '\\'
        ensure_stream(i + 1);
        if (is_alpha(c) || (c=='\\' && (i+1<stream.size()) && (stream[i+1]=='u' || stream[i+1]=='U'))) {
            size_t j = i;
            bool validStart = false;
            // consume first char or require valid UCN
            if (is_alpha(stream[j])) { 
                if (j + 1 > stream.size()) ensure_stream(j + 1);
                ++j; 
                validStart = true;
            } else {
                size_t u = 0; if (try_ucn(j, u)) { j += u; validStart = true; }
            }
            if (validStart) {
                // consume subsequent identifier chars: alnum or underscores or ucn
                while (true) {
                    if (j >= stream.size()) ensure_stream(j);
                    if (j >= stream.size()) break;
                    if (is_alnum_us(stream[j])) { ++j; continue; }
                    size_t u = 0; if (try_ucn(j, u)) { j += u; continue; }
                    break;
                }
                if (j > i) {
                    std::string lex = stream.substr(i, j - i);
                    emit(PPTokenKind::Identifier, lex, begin, feeder.position());
                    i = j; continue;
                }
            }
        }

        size_t plen = 0;
        if (i < stream.size()) {
            // Ensure enough lookahead for longest punctuators (e.g., "%:%:" is 4 chars)
            if (stream.size() - i < 4) ensure_stream(i + 3);
            plen = match_punct(stream, i);
        }
        if (plen > 0) {
            std::string lex = stream.substr(i, plen);
            i += plen;
            pos.column += (int)plen;
            pos.offset += plen;
            emit(PPTokenKind::Punctuator, lex, begin, feeder.position());
            continue;
        }

        // Fallback: single Other character
        std::string lex(1, c);
        ++i;
        emit(PPTokenKind::Other, lex, begin, feeder.position());
    }

    return out;
}

} // namespace wvmcc
