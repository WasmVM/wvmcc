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
    auto emit = [&](PPTokenKind kind, const std::string& lexeme, SourcePos begin, SourcePos end) {
        out.push_back(PPToken{kind, SourceSpan{begin, end}, lexeme});
    };

    size_t i = 0;
    while (i < input.size()) {
        char c = input[i];
        SourcePos begin = pos;

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
