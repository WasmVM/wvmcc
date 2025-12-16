#include "Tokenizer.hpp"
#include <cstring>
#include <istream>
#include <deque>
#include <regex>

namespace wvmcc {

// Regex-based punctuator matcher - CCN 3, matches C punctuators greedily by length
static size_t match_punct(const std::string& s, size_t i) {
    if (i >= s.size()) return 0;
    
    // Pattern ordered by length (4→3→2→1 char) for greedy matching
    static const std::regex punct_re(
        R"(^(%:%:|[<>]{2}=|\.\.\.|%[:>=]|<[:<%<=]|>[>=]|:>|##|->|-[-=]|\+[\+=]|&[&=]|\|[\|=]|[!=\*/^]=|[%<>:#\-+&|*/^!=.\[\](){}~,;?]))"
    );
    
    std::smatch m;
    std::string tail_str(s.data() + i, s.size() - i);
    
    if (std::regex_search(tail_str, m, punct_re)) {
        return m[0].length();
    }
    return 0;
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

bool SourceBuffer::handlePendingSpace() {
    if (!pendingSpace) return false;
    if (!lastOutputWasWhitespace) {
        charBuf.push_back(' ');
        lastOutputWasWhitespace = true;
    }
    pendingSpace = false;
    return true;
}

bool SourceBuffer::handleCarriageReturn() {
    char c = inputAccum[rawIdx];
    if (c != '\r') return false;
    
    if (rawIdx + 1 < inputAccum.size() && inputAccum[rawIdx+1] == '\n') {
        rawIdx += 2;
    } else {
        ++rawIdx;
    }
    charBuf.push_back('\n');
    lastOutputWasWhitespace = true;
    return true;
}

bool SourceBuffer::tryProcessComment(char c) {
    if (c != '/' || rawIdx + 1 >= inputAccum.size()) return false;
    
    char n = inputAccum[rawIdx+1];
    if (n == '*') {
        st = State::InBlockComment;
        rawIdx += 2;
        return true;
    }
    if (n == '/') {
        st = State::InLineComment;
        rawIdx += 2;
        return true;
    }
    return false;
}

bool SourceBuffer::tryProcessLineSplicing(char c) {
    if (c != '\\' || rawIdx + 1 >= inputAccum.size()) return false;
    
    if (inputAccum[rawIdx+1] == '\n') {
        rawIdx += 2;
        return true;
    }
    if (inputAccum[rawIdx+1] == '\r') {
        if (rawIdx + 2 < inputAccum.size() && inputAccum[rawIdx+2] == '\n') {
            rawIdx += 3;
        } else {
            rawIdx += 2;
        }
        return true;
    }
    return false;
}

bool SourceBuffer::processNormalState(char c) {
    // trigraphs
    char tr = trigraph_at(rawIdx);
    if (tr) {
        rawIdx += 3;
        charBuf.push_back(tr);
        return true;
    }
    
    // enter string/char literals
    if (c == '"') {
        charBuf.push_back(c);
        lastOutputWasWhitespace = false;
        ++rawIdx;
        st = State::InString;
        esc = false;
        return true;
    }
    if (c == '\'') {
        charBuf.push_back(c);
        lastOutputWasWhitespace = false;
        ++rawIdx;
        st = State::InChar;
        esc = false;
        return true;
    }
    
    // comments
    if (tryProcessComment(c)) return false;
    
    // line splicing
    if (tryProcessLineSplicing(c)) return false;
    
    // normal emission
    charBuf.push_back(c);
    lastOutputWasWhitespace = (c == ' ' || c == '\t' || c == '\v' || c == '\f');
    ++rawIdx;
    return true;
}

bool SourceBuffer::processStringState(char c) {
    charBuf.push_back(c);
    lastOutputWasWhitespace = false;
    ++rawIdx;
    
    if (!esc) {
        if (c == '\\') {
            esc = true;
        } else if (c == '"') {
            st = State::Normal;
        }
    } else {
        esc = false;
    }
    return true;
}

bool SourceBuffer::processCharState(char c) {
    charBuf.push_back(c);
    lastOutputWasWhitespace = false;
    ++rawIdx;
    
    if (!esc) {
        if (c == '\\') {
            esc = true;
        } else if (c == '\'') {
            st = State::Normal;
        }
    } else {
        esc = false;
    }
    return true;
}

bool SourceBuffer::processBlockCommentState(char c) {
    if (c == '*' && rawIdx + 1 < inputAccum.size() && inputAccum[rawIdx+1] == '/') {
        rawIdx += 2;
        st = State::Normal;
        pendingSpace = true;
        return true;
    }
    ++rawIdx;
    return false; // continue outer loop
}

bool SourceBuffer::processLineCommentState(char c) {
    if (c == '\n') {
        ++rawIdx;
        charBuf.push_back('\n');
        lastOutputWasWhitespace = true;
        st = State::Normal;
        return true;
    }
    ++rawIdx;
    return false; // continue outer loop
}

void SourceBuffer::fill_buffer() {
    while (charBuf.empty()) {
        ensure_input(rawIdx + 3);
        if (rawIdx >= inputAccum.size()) break;
        
        char c = inputAccum[rawIdx];
        
        if (handlePendingSpace()) break;
        if (handleCarriageReturn()) break;
        
        bool shouldBreak = false;
        if (st == State::Normal) {
            shouldBreak = processNormalState(c);
        } else if (st == State::InString) {
            shouldBreak = processStringState(c);
        } else if (st == State::InChar) {
            shouldBreak = processCharState(c);
        } else if (st == State::InBlockComment) {
            shouldBreak = processBlockCommentState(c);
        } else if (st == State::InLineComment) {
            shouldBreak = processLineCommentState(c);
        }
        
        if (shouldBreak) break;
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

    // Try each token type in priority order
    if (tryReadPPNumber(out, c, begin)) return true;
    if (tryReadCharConstant(out, begin)) return true;
    if (tryReadStringLiteral(out, begin)) return true;
    if (tryReadNewline(out, c, begin)) return true;
    if (tryReadWhitespace(out, c, begin)) return true;
    if (tryReadIdentifier(out, c, begin)) return true;
    if (tryReadPunctuator(out, begin)) return true;
    
    // Fallback: Other token
    readOtherToken(out, begin);
    return true;
}

bool Tokenizer::tryConsumeInitialDot(std::string& lex) {
    auto c1 = feeder.peek(1);
    if (c1.has_value() && is_digit(c1.value())) {
        feeder.consume(1);
        lex.push_back('.');
        return true;
    }
    return false;
}

bool Tokenizer::tryConsumeInitialDigit(std::string& lex) {
    auto d0 = feeder.peek(0);
    if (d0.has_value() && is_digit(d0.value())) {
        lex.push_back(d0.value());
        feeder.consume(1);
        return true;
    }
    return false;
}

void Tokenizer::consumeDigitsAfterExponent(std::string& lex) {
    while (true) {
        auto nd = feeder.peek(0);
        if (nd.has_value() && is_digit(nd.value())) {
            lex.push_back(nd.value());
            feeder.consume(1);
        } else break;
    }
}

bool Tokenizer::processExponentNotation(char d, std::string& lex) {
    if (d != 'e' && d != 'E' && d != 'p' && d != 'P') {
        return false;
    }
    feeder.consume(1);
    lex.push_back(d);
    auto sign = feeder.peek(0);
    if (sign.has_value() && (sign.value() == '+' || sign.value() == '-')) {
        lex.push_back(sign.value());
        feeder.consume(1);
    }
    consumeDigitsAfterExponent(lex);
    return true;
}

bool Tokenizer::tryReadPPNumber(PPToken& out, char c, const SourcePos& begin) {
    auto c1 = feeder.peek(1);
    bool isPPNumber = (c == '.' && c1.has_value() && is_digit(c1.value())) || is_digit(c);
    if (!isPPNumber) return false;

    std::string lex;
    if (c == '.') {
        tryConsumeInitialDot(lex);
    }
    tryConsumeInitialDigit(lex);
    
    while (true) {
        auto dopt = feeder.peek(0);
        if (!dopt.has_value()) break;
        char d = dopt.value();
        
        if (processExponentNotation(d, lex)) {
            continue;
        }
        if (is_digit(d) || is_nondigit(d)) {
            lex.push_back(d);
            feeder.consume(1);
            continue;
        }
        if (d == '.') {
            lex.push_back('.');
            feeder.consume(1);
            continue;
        }
        break;
    }
    
    out = PPToken{PPTokenKind::PPNumber, SourceSpan{begin, feeder.position()}, lex};
    return true;
}

bool Tokenizer::processEscapeHex(std::string& lex, bool& escaped) {
    auto ch = feeder.get();
    if (ch) lex.push_back(ch.value());
    while (true) {
        auto hx = feeder.peek(0);
        if (hx && is_hex(hx.value())) {
            auto cc = feeder.get();
            if (cc) lex.push_back(cc.value());
        } else break;
    }
    escaped = false;
    return true;
}

bool Tokenizer::processEscapeUnicode(std::string& lex, bool& escaped, int count) {
    auto ch = feeder.get();
    if (ch) lex.push_back(ch.value());
    for (int k = 0; k < count; ++k) {
        auto hx = feeder.peek(0);
        if (hx && is_hex(hx.value())) {
            auto cc = feeder.get();
            if (cc) lex.push_back(cc.value());
        }
    }
    escaped = false;
    return true;
}

bool Tokenizer::processEscapeOctal(std::string& lex, bool& escaped) {
    auto ch = feeder.get();
    if (ch) lex.push_back(ch.value());
    for (int k = 1; k < 3; ++k) {
        auto oc = feeder.peek(0);
        if (oc && is_oct(oc.value())) {
            auto cc = feeder.get();
            if (cc) lex.push_back(cc.value());
        }
    }
    escaped = false;
    return true;
}

bool Tokenizer::processEscapedChar(char d, std::string& lex, bool& escaped) {
    if (d == 'x') return processEscapeHex(lex, escaped);
    if (d == 'u') return processEscapeUnicode(lex, escaped, 4);
    if (d == 'U') return processEscapeUnicode(lex, escaped, 8);
    if (is_oct(d)) return processEscapeOctal(lex, escaped);
    
    auto ch2 = feeder.get();
    if (ch2) lex.push_back(ch2.value());
    escaped = false;
    return true;
}

bool Tokenizer::tryReadCharConstant(PPToken& out, const SourcePos& begin) {
    size_t charPrefixLen = 0;
    if (!starts_char(0, charPrefixLen)) return false;

    std::string lex;
    for (size_t k = 0; k < charPrefixLen; ++k) {
        auto ch = feeder.get();
        if (ch) lex.push_back(ch.value());
    }
    
    auto q = feeder.get();
    if (q) lex.push_back(q.value());
    
    bool escaped = false;
    while (true) {
        auto dopt = feeder.peek(0);
        if (!dopt.has_value()) break;
        char d = dopt.value();
        
        if (!escaped) {
            if (d == '\\') {
                auto ch = feeder.get();
                if (ch) lex.push_back(ch.value());
                escaped = true;
                continue;
            }
            if (d == '\'') {
                auto ch = feeder.get();
                if (ch) lex.push_back(ch.value());
                break;
            }
            if (d == '\n') break;
            auto ch = feeder.get();
            if (ch) lex.push_back(ch.value());
        } else {
            processEscapedChar(d, lex, escaped);
        }
    }
    
    out = PPToken{PPTokenKind::CharConst, SourceSpan{begin, feeder.position()}, lex};
    return true;
}

void Tokenizer::consumeStringPrefix(std::string& lex, size_t prefixLen) {
    for (size_t k = 0; k < prefixLen; ++k) {
        auto ch = feeder.get();
        if (ch) lex.push_back(ch.value());
    }
}

void Tokenizer::consumeOpeningQuote(std::string& lex) {
    auto q = feeder.get();
    if (q) lex.push_back(q.value());
}

void Tokenizer::processStringContent(std::string& lex) {
    bool escaped = false;
    while (true) {
        auto dopt = feeder.peek(0);
        if (!dopt.has_value()) break;
        char d = dopt.value();
        
        if (!escaped) {
            if (d == '\\') {
                auto ch = feeder.get();
                if (ch) lex.push_back(ch.value());
                escaped = true;
                continue;
            }
            if (d == '"') {
                auto ch = feeder.get();
                if (ch) lex.push_back(ch.value());
                break;
            }
            if (d == '\n') break;
            auto ch = feeder.get();
            if (ch) lex.push_back(ch.value());
        } else {
            auto ch = feeder.get();
            if (ch) lex.push_back(ch.value());
            escaped = false;
        }
    }
}

bool Tokenizer::tryReadStringLiteral(PPToken& out, const SourcePos& begin) {
    size_t prefixLen = 0;
    if (!starts_string(0, prefixLen)) return false;

    std::string lex;
    consumeStringPrefix(lex, prefixLen);
    consumeOpeningQuote(lex);
    processStringContent(lex);
    
    out = PPToken{PPTokenKind::StringLiteral, SourceSpan{begin, feeder.position()}, lex};
    return true;
}

bool Tokenizer::tryReadNewline(PPToken& out, char c, const SourcePos& begin) {
    if (c != '\n') return false;
    
    feeder.consume(1);
    out = PPToken{PPTokenKind::Newline, SourceSpan{begin, feeder.position()}, "\n"};
    return true;
}

bool Tokenizer::tryReadWhitespace(PPToken& out, char c, const SourcePos& begin) {
    if (!is_space(c)) return false;
    
    std::string lex;
    do {
        auto ch = feeder.get();
        if (ch) lex.push_back(ch.value());
        auto np = feeder.peek(0);
        if (!np.has_value()) break;
        c = np.value();
    } while (is_space(c));
    
    out = PPToken{PPTokenKind::Whitespace, SourceSpan{begin, feeder.position()}, lex};
    return true;
}

bool Tokenizer::canStartIdentifier(char c) {
    auto n1 = feeder.peek(1);
    return is_nondigit(c) || (c == '\\' && n1.has_value() && 
                              (n1.value() == 'u' || n1.value() == 'U'));
}

bool Tokenizer::consumeUCN(std::string& lex) {
    size_t u = 0;
    if (!try_ucn(0, u)) return false;
    
    for (size_t k = 0; k < u; ++k) {
        auto ch = feeder.get();
        if (ch) lex.push_back(ch.value());
    }
    return true;
}

bool Tokenizer::tryConsumeIdentifierStart(char c, std::string& lex) {
    if (is_nondigit(c)) {
        auto ch = feeder.get();
        if (ch) {
            lex.push_back(ch.value());
            return true;
        }
        return false;
    }
    return consumeUCN(lex);
}

void Tokenizer::consumeIdentifierContinuation(std::string& lex) {
    while (true) {
        auto p = feeder.peek(0);
        if (!p.has_value()) break;
        char d = p.value();
        
        if (is_nondigit(d) || is_digit(d)) {
            feeder.get();
            lex.push_back(d);
            continue;
        }
        
        if (consumeUCN(lex)) {
            continue;
        }
        break;
    }
}

bool Tokenizer::tryReadIdentifier(PPToken& out, char c, const SourcePos& begin) {
    if (!canStartIdentifier(c)) return false;

    std::string lex;
    if (!tryConsumeIdentifierStart(c, lex)) return false;
    
    consumeIdentifierContinuation(lex);
    
    out = PPToken{PPTokenKind::Identifier, SourceSpan{begin, feeder.position()}, lex};
    return true;
}

bool Tokenizer::tryReadPunctuator(PPToken& out, const SourcePos& begin) {
    std::string snapshot;
    for (size_t i = 0; i < 4; ++i) {
        auto ch = feeder.peek(i);
        if (ch) snapshot.push_back(ch.value());
        else break;
    }
    
    size_t plen = match_punct(snapshot, 0);
    if (plen == 0) return false;
    
    std::string lex;
    for (size_t i = 0; i < plen; ++i) {
        auto ch = feeder.get();
        if (ch) lex.push_back(ch.value());
    }
    
    out = PPToken{PPTokenKind::Punctuator, SourceSpan{begin, feeder.position()}, lex};
    return true;
}

void Tokenizer::readOtherToken(PPToken& out, const SourcePos& begin) {
    std::string lex;
    auto gc = feeder.get();
    if (gc) lex.push_back(gc.value());
    out = PPToken{PPTokenKind::Other, SourceSpan{begin, feeder.position()}, lex};
}

} // namespace wvmcc
