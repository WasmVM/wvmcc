#include "Tokenizer.hpp"
#include <cstring>
#include <istream>
#include <deque>

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

SourceBuffer::SourceBuffer(std::istream& in) : inStream(in) {
    charBuf.reserve(64);
}

char SourceBuffer::trigraph_at(std::size_t idx) const {
    if (idx + 2 < inputAccum.size() && inputAccum[idx] == '?' && inputAccum[idx+1] == '?') {
        switch (inputAccum[idx+2]) {
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
    while (charBuf.empty()) {
        ensure_input(rawIdx + 3);
        if (rawIdx >= inputAccum.size()) break;
        char c = inputAccum[rawIdx];
        // If a pending separating space is requested, emit exactly one space
        // Skip inserting if we already emitted whitespace just before
        if (pendingSpace) {
            if (!lastOutputWasWhitespace) {
                charBuf.push_back(' ');
                lastOutputWasWhitespace = true;
            }
            pendingSpace = false;
            break;
        }
        // EOL normalize
        if (c == '\r') {
            if (rawIdx + 1 < inputAccum.size() && inputAccum[rawIdx+1] == '\n') { rawIdx += 2; charBuf.push_back('\n'); }
            else { ++rawIdx; charBuf.push_back('\n'); }
            lastOutputWasWhitespace = true;
            break;
        }
        if (st == State::Normal) {
            // trigraphs
            char tr = trigraph_at(rawIdx);
            if (tr) { rawIdx += 3; charBuf.push_back(tr); break; }
            // enter string/char literals
            if (c == '"') { charBuf.push_back(c); lastOutputWasWhitespace = false; ++rawIdx; st = State::InString; esc = false; break; }
            if (c == '\'') { charBuf.push_back(c); lastOutputWasWhitespace = false; ++rawIdx; st = State::InChar; esc = false; break; }
            // comments
            if (c == '/' && rawIdx + 1 < inputAccum.size()) {
                char n = inputAccum[rawIdx+1];
                if (n == '*') { st = State::InBlockComment; rawIdx += 2; continue; }
                if (n == '/') { st = State::InLineComment; rawIdx += 2; continue; }
            }
            // line splicing (remove backslash-newline without inserting space)
            if (c == '\\') {
                if (rawIdx + 1 < inputAccum.size()) {
                    if (inputAccum[rawIdx+1] == '\n') { rawIdx += 2; continue; }
                    if (inputAccum[rawIdx+1] == '\r') {
                        if (rawIdx + 2 < inputAccum.size() && inputAccum[rawIdx+2] == '\n') { rawIdx += 3; continue; }
                        rawIdx += 2; continue;
                    }
                }
            }
            // normal emission
            charBuf.push_back(c);
            lastOutputWasWhitespace = (c == ' ' || c == '\t' || c == '\v' || c == '\f');
            ++rawIdx;
            break;
        } else if (st == State::InString) {
            charBuf.push_back(c);
            lastOutputWasWhitespace = false;
            ++rawIdx;
            if (!esc) { if (c == '\\') esc = true; else if (c == '"') st = State::Normal; }
            else { esc = false; }
            break;
        } else if (st == State::InChar) {
            charBuf.push_back(c);
            lastOutputWasWhitespace = false;
            ++rawIdx;
            if (!esc) { if (c == '\\') esc = true; else if (c == '\'') st = State::Normal; }
            else { esc = false; }
            break;
        } else if (st == State::InBlockComment) {
            if (c == '*' && rawIdx + 1 < inputAccum.size() && inputAccum[rawIdx+1] == '/') {
                rawIdx += 2;
                st = State::Normal;
                // Always insert a single separating space after removing a block comment
                pendingSpace = true;
                break;
            }
            ++rawIdx; continue;
        } else if (st == State::InLineComment) {
            if (c == '\n') { ++rawIdx; charBuf.push_back('\n'); lastOutputWasWhitespace = true; st = State::Normal; break; }
            ++rawIdx; continue;
        }
    }
    // At end of raw input and no buffered output, stop; do not synthesize final newline.
    if (rawIdx >= inputAccum.size() && charBuf.empty()) {
        if (!eof) return; // more input may still arrive
        return; // true EOF reached
    }
}

bool SourceBuffer::next_char(char& outCh) {
    if (charBuf.empty()) fill_buffer();
    if (charBuf.empty()) return false;
    outCh = charBuf.front();
    charBuf.erase(charBuf.begin());
    return true;
}

// Removed legacy ensure_stream; Tokenizer now uses ring buffer API.

void SourceBuffer::ensure_input(std::size_t upto) {
    while (inputAccum.size() <= upto) {
        int c = inStream.get();
        if (c == EOF) { eof = true; break; }
        inputAccum.push_back(static_cast<char>(c));
    }
}

void SourceBuffer::reset() {
    st = State::Normal;
    esc = false;
    rawIdx = 0;
    lastOutputWasWhitespace = false;
    inStream.clear();
    inStream.seekg(0, std::ios::beg);
    inputAccum.clear();
    eof = false;
    charBuf.clear();
    ring.clear();
    pendingSpace = false;
    pos = SourcePos{0, 1, 1, 0};
}

// Ring buffer lookahead API implementations
void SourceBuffer::ensure(std::size_t k) {
    while (ring.size() < k) {
        char ch;
        if (!next_char(ch)) break;
        ring.push_back(ch);
    }
}

std::optional<char> SourceBuffer::peek(std::size_t i) {
    ensure(i + 1);
    if (i < ring.size()) return ring[i];
    return std::nullopt;
}

bool SourceBuffer::consume(std::size_t n) {
    ensure(n);
    if (ring.size() < n) return false;
    for (std::size_t i = 0; i < n; ++i) {
        char ch = ring.front();
        ring.pop_front();
        account_consumed(ch);
    }
    return true;
}

std::optional<char> SourceBuffer::get() {
    ensure(1);
    if (ring.empty()) return std::nullopt;
    char c = ring.front();
    ring.pop_front();
    account_consumed(c);
    return c;
}

void SourceBuffer::account_consumed(char ch) {
    if (ch == '\n') { ++pos.line; pos.column = 1; }
    else { ++pos.column; }
    ++pos.offset;
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

Tokenizer::Tokenizer(std::istream& in) : feeder(in) {}

bool Tokenizer::try_ucn(size_t idx, size_t& consumed) {
    consumed = 0;
    auto c0 = feeder.peek(idx);
    if (!c0.has_value() || c0.value() != '\\') return false;
    auto c1 = feeder.peek(idx + 1);
    if (c1.has_value() && c1.value() == 'u') {
        // \\uXXXX (exactly 4 hex digits)
        for (size_t k = 0; k < 4; ++k) {
            auto ch = feeder.peek(idx + 2 + k);
            if (!ch.has_value() || !Tokenizer::is_hex(ch.value())) return false;
        }
        consumed = 6; return true;
    }
    if (c1.has_value() && c1.value() == 'U') {
        // \\UXXXXXXXX (exactly 8 hex digits)
        for (size_t k = 0; k < 8; ++k) {
            auto ch = feeder.peek(idx + 2 + k);
            if (!ch.has_value() || !Tokenizer::is_hex(ch.value())) return false;
        }
        consumed = 10; return true;
    }
    return false;
}

bool Tokenizer::starts_char(size_t idx, size_t& prefixLen) {
    prefixLen = 0;
    auto c0 = feeder.peek(idx);
    if (!c0.has_value()) return false;
    if (c0.value() == '\'') { prefixLen = 0; return true; }
    auto c1 = feeder.peek(idx + 1);
    if ((c0.value() == 'L' || c0.value() == 'u' || c0.value() == 'U') && c1.has_value() && c1.value() == '\'') { prefixLen = 1; return true; }
    return false;
}

bool Tokenizer::starts_string(size_t idx, size_t& prefixLen) {
    prefixLen = 0;
    auto c0 = feeder.peek(idx);
    if (!c0.has_value()) return false;
    if (c0.value() == '"') { prefixLen = 0; return true; }
    if (c0.value() == 'u') {
        auto c1 = feeder.peek(idx + 1);
        auto c2 = feeder.peek(idx + 2);
        if (c1.has_value() && c1.value() == '8' && c2.has_value() && c2.value() == '"') { prefixLen = 2; return true; }
        if (c1.has_value() && c1.value() == '"') { prefixLen = 1; return true; }
    } else if (c0.value() == 'U' || c0.value() == 'L') {
        auto c1 = feeder.peek(idx + 1);
        if (c1.has_value() && c1.value() == '"') { prefixLen = 1; return true; }
    }
    return false;
}


void Tokenizer::reset() {
    feeder.reset();
    lookahead.reset();
}

std::optional<PPToken> Tokenizer::next() {
    if (lookahead.has_value()) {
        PPToken tok = *lookahead;
        lookahead.reset();
        return tok;
    }
    PPToken tok;
    if (!readNextToken(tok)) return std::nullopt;
    return tok;
}

std::optional<PPToken> Tokenizer::peek() {
    if (lookahead.has_value()) return lookahead;
    PPToken tok;
    if (!readNextToken(tok)) return std::nullopt;
    lookahead = tok;
    return lookahead;
}

void Tokenizer::Iterator::advance() {
    if (!tz) { eof = true; current.reset(); return; }
    auto nextTok = tz->next();
    if (!nextTok.has_value()) { eof = true; current.reset(); }
    else { eof = false; current = nextTok; }
}

Tokenizer::Iterator Tokenizer::begin() {
    reset();
    return Iterator(this);
}

Tokenizer::Iterator Tokenizer::end() {
    return Iterator();
}

bool Tokenizer::readNextToken(PPToken& out) {
    auto c0 = feeder.peek(0);
    if (!c0.has_value()) return false;
    char c = c0.value();
    SourcePos begin = feeder.position();

    // Preprocessing number (PPNumber)
    auto c1 = feeder.peek(1);
    if (c == '.' ? (c1.has_value() && is_digit(c1.value())) : is_digit(c)) {
        std::string lex;
        if (c == '.') { feeder.consume(1); lex.push_back('.'); }
        auto d0 = feeder.peek(0);
        if (d0.has_value() && is_digit(d0.value())) { lex.push_back(d0.value()); feeder.consume(1); }
        while (true) {
            auto dopt = feeder.peek(0);
            if (!dopt.has_value()) break;
            char d = dopt.value();
            if (d=='e'||d=='E'||d=='p'||d=='P') {
                feeder.consume(1); lex.push_back(d);
                auto sign = feeder.peek(0);
                if (sign.has_value() && (sign.value() == '+' || sign.value() == '-')) { lex.push_back(sign.value()); feeder.consume(1); }
                while (true) {
                    auto nd = feeder.peek(0);
                    if (nd.has_value() && (nd.value() >= '0' && nd.value() <= '9')) { lex.push_back(nd.value()); feeder.consume(1); }
                    else break;
                }
                continue;
            }
            if (is_digit(d) || is_nondigit(d)) { lex.push_back(d); feeder.consume(1); continue; }
            if (d == '.') { lex.push_back('.'); feeder.consume(1); continue; }
            break;
        }
        out = PPToken{PPTokenKind::PPNumber, SourceSpan{begin, feeder.position()}, lex};
        return true;
    }

    // Character constant (with optional encoding prefix L/u/U)
    size_t charPrefixLen = 0;
    if (starts_char(0, charPrefixLen)) {
        std::string lex;
        for (size_t k = 0; k < charPrefixLen; ++k) { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); }
        auto q = feeder.get(); if (q) lex.push_back(q.value());
        bool escaped = false;
        while (true) {
            auto dopt = feeder.peek(0);
            if (!dopt.has_value()) break;
            char d = dopt.value();
            if (!escaped) {
                if (d == '\\') { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); escaped = true; continue; }
                if (d == '\'') { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); break; }
                if (d == '\n') { break; }
                auto ch = feeder.get(); if (ch) lex.push_back(ch.value());
            } else {
                if (d == 'x') { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); while (true) { auto hx = feeder.peek(0); if (hx && is_hex(hx.value())) { auto cc = feeder.get(); if (cc) lex.push_back(cc.value()); } else break; } escaped = false; continue; }
                if (d == 'u') { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); for (int k=0;k<4; ++k) { auto hx = feeder.peek(0); if (hx && is_hex(hx.value())) { auto cc = feeder.get(); if (cc) lex.push_back(cc.value()); } } escaped = false; continue; }
                if (d == 'U') { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); for (int k=0;k<8; ++k) { auto hx = feeder.peek(0); if (hx && is_hex(hx.value())) { auto cc = feeder.get(); if (cc) lex.push_back(cc.value()); } } escaped = false; continue; }
                if (is_oct(d)) { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); for (int k=1;k<3; ++k) { auto oc = feeder.peek(0); if (oc && is_oct(oc.value())) { auto cc = feeder.get(); if (cc) lex.push_back(cc.value()); } } escaped = false; continue; }
                auto ch2 = feeder.get(); if (ch2) lex.push_back(ch2.value()); escaped = false;
            }
        }
        out = PPToken{PPTokenKind::CharConst, SourceSpan{begin, feeder.position()}, lex};
        return true;
    }

    // String literal (with optional encoding prefix u8/u/U/L)
    size_t prefixLen = 0;
    if (starts_string(0, prefixLen)) {
        std::string lex;
        for (size_t k = 0; k < prefixLen; ++k) { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); }
        auto q = feeder.get(); if (q) lex.push_back(q.value());
        bool escaped = false;
        while (true) {
            auto dopt = feeder.peek(0);
            if (!dopt.has_value()) break;
            char d = dopt.value();
            if (!escaped) {
                if (d == '\\') { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); escaped = true; continue; }
                if (d == '"') { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); break; }
                if (d == '\n') { break; }
                auto ch = feeder.get(); if (ch) lex.push_back(ch.value());
            } else {
                auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); escaped = false;
            }
        }
        out = PPToken{PPTokenKind::StringLiteral, SourceSpan{begin, feeder.position()}, lex};
        return true;
    }

    // Newline
    if (c == '\n') {
        feeder.consume(1);
        out = PPToken{PPTokenKind::Newline, SourceSpan{begin, feeder.position()}, "\n"};
        return true;
    }

    // Whitespace
    if (is_space(c)) {
        std::string lex;
        do {
            auto ch = feeder.get(); if (ch) lex.push_back(ch.value());
            auto np = feeder.peek(0); if (!np.has_value()) break; c = np.value();
        } while (is_space(c));
        out = PPToken{PPTokenKind::Whitespace, SourceSpan{begin, feeder.position()}, lex};
        return true;
    }

    // Identifier: starts with nondigit or universal-character-name
    auto n1 = feeder.peek(1);
    if (is_nondigit(c) || (c=='\\' && n1.has_value() && (n1.value()=='u' || n1.value()=='U'))) {
        bool validStart = false;
        std::string lex;
        if (is_nondigit(c)) { auto ch = feeder.get(); if (ch) { lex.push_back(ch.value()); validStart = true; } }
        else { size_t u = 0; if (try_ucn(0, u)) { for (size_t k=0;k<u;++k){ auto ch2 = feeder.get(); if (ch2) lex.push_back(ch2.value()); } validStart = true; } }
        if (validStart) {
            while (true) {
                auto p = feeder.peek(0);
                if (!p.has_value()) break;
                char d = p.value();
                if (is_nondigit(d) || is_digit(d)) { auto ch3 = feeder.get(); (void)ch3; lex.push_back(d); continue; }
                size_t u = 0; if (try_ucn(0, u)) { for (size_t k=0;k<u;++k){ auto ch4 = feeder.get(); if (ch4) lex.push_back(ch4.value()); } continue; }
                break;
            }
            out = PPToken{PPTokenKind::Identifier, SourceSpan{begin, feeder.position()}, lex};
            return true;
        }
    }

    // Punctuator: greedy longest-match over up to 4 chars
    std::string snapshot;
    for (size_t i=0;i<4;++i) { auto ch = feeder.peek(i); if (ch) snapshot.push_back(ch.value()); else break; }
    size_t plen = match_punct(snapshot, 0);
    if (plen > 0) {
        std::string lex;
        for (size_t i=0;i<plen;++i) { auto ch = feeder.get(); if (ch) lex.push_back(ch.value()); }
        out = PPToken{PPTokenKind::Punctuator, SourceSpan{begin, feeder.position()}, lex};
        return true;
    }

    // Fallback: single Other character
    std::string lex;
    auto gc = feeder.get(); if (gc) lex.push_back(gc.value());
    out = PPToken{PPTokenKind::Other, SourceSpan{begin, feeder.position()}, lex};
    return true;
}

} // namespace wvmcc
