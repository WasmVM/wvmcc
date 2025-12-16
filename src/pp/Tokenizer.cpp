#include "Tokenizer.hpp"

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

std::vector<PPToken> Tokenizer::tokenize_with_punctuators(const std::string& input) {
    std::vector<PPToken> out;
    out.reserve(input.size() / 2);

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

    size_t i = 0;
    while (i < input.size()) {
        char c = input[i];
        SourcePos begin = pos;

        // String literal (with optional encoding prefix u8/u/U/L)
        auto starts_string = [&](size_t idx, size_t& prefixLen) -> bool {
            prefixLen = 0;
            if (idx >= input.size()) return false;
            if (input[idx] == '"') { prefixLen = 0; return true; }
            if (input[idx] == 'u') {
                if (idx + 2 < input.size() && input[idx + 1] == '8' && input[idx + 2] == '"') { prefixLen = 2; return true; }
                if (idx + 1 < input.size() && input[idx + 1] == '"') { prefixLen = 1; return true; }
            } else if (input[idx] == 'U' || input[idx] == 'L') {
                if (idx + 1 < input.size() && input[idx + 1] == '"') { prefixLen = 1; return true; }
            }
            return false;
        };
        size_t prefixLen = 0;
        if (starts_string(i, prefixLen)) {
            size_t j = i;
            // Emit full literal from start of prefix to ending quote
            // Advance through prefix and opening quote updating pos
            for (size_t k = 0; k < prefixLen; ++k) { advance_pos(input[j]); ++j; }
            // opening quote
            if (j < input.size() && input[j] == '"') { advance_pos('"'); ++j; }
            bool escaped = false;
            while (j < input.size()) {
                char d = input[j];
                if (!escaped) {
                    if (d == '\\') {
                        advance_pos(d); ++j; escaped = true; continue;
                    }
                    if (d == '"') {
                        // include closing quote
                        advance_pos(d); ++j; break;
                    }
                    if (d == '\n') {
                        // Invalid: newline terminates string literal (do not consume) per C spec
                        break;
                    }
                    advance_pos(d); ++j; // normal char
                } else {
                    // escaped character
                    advance_pos(d); ++j; escaped = false;
                }
            }
            std::string lex = input.substr(i, j - i);
            emit(PPTokenKind::StringLiteral, lex, begin, pos);
            i = j;
            continue;
        }

        if (c == '\n') {
            ++i; ++pos.line; pos.column = 1; ++pos.offset;
            emit(PPTokenKind::Newline, "\n", begin, pos);
            continue;
        }

        if (is_space(c)) {
            std::string lex;
            do { lex.push_back(c); ++i; ++pos.column; ++pos.offset; if (i>=input.size()) break; c = input[i]; } while (is_space(c));
            emit(PPTokenKind::Whitespace, lex, begin, pos);
            continue;
        }

        size_t plen = match_punct(input, i);
        if (plen > 0) {
            std::string lex = input.substr(i, plen);
            i += plen;
            pos.column += (int)plen;
            pos.offset += plen;
            emit(PPTokenKind::Punctuator, lex, begin, pos);
            continue;
        }

        // Fallback: single Other character
        std::string lex(1, c);
        ++i; ++pos.column; ++pos.offset;
        emit(PPTokenKind::Other, lex, begin, pos);
    }

    return out;
}

} // namespace wvmcc
