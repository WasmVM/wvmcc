// Lexer implementation moved out of header
#include "Lexer.hpp"
#include <unordered_set>

namespace wvmcc::parser {

static const std::unordered_set<std::string> keywords = {
    "auto","extern","short","while","break","float","signed",
    "_Alignas","case","for","sizeof","_Alignof","char","goto",
    "static","_Atomic","const","if","struct","_Bool","continue",
    "inline","switch","_Complex","default","int","typedef",
    "_Generic","do","long","union","_Imaginary","double","register",
    "unsigned","_Noreturn","else","restrict","void","_Static_assert",
    "enum","return","volatile","_Thread_local"
};

// Classification helper used internally by this translation unit.
static inline Token classify_local(const wvmcc::PPToken& pp) {
    using K = wvmcc::PPTokenKind;
    switch (pp.kind) {
        case K::Identifier:
            if (keywords.count(pp.lexeme)) return Token(KeywordToken{pp.lexeme}, pp.span);
            return Token(IdentifierToken{pp.lexeme}, pp.span);
        case K::PPNumber: {
            // Heuristic: if lexeme contains '.', 'e', 'E', 'p', or 'P' it's a floating constant
            const std::string& s = pp.lexeme;
            bool isFloat = false;
            for (char c : s) {
                if (c == '.' || c == 'e' || c == 'E' || c == 'p' || c == 'P') { isFloat = true; break; }
            }
            if (isFloat) return Token(FloatingToken{pp.lexeme}, pp.span);

            // Parse integer constant: split suffix (u/U, l/L, ll/LL) from digits
            size_t j = s.size();
            bool isUnsigned = false;
            int longCount = 0;
            // consume trailing u/U and l/L in any valid order
            while (j > 0) {
                char c = s[j-1];
                if (c == 'u' || c == 'U') { isUnsigned = true; --j; continue; }
                if (c == 'l' || c == 'L') {
                    // count up to two L's for long-long
                    int cnt = 0;
                    while (j > 0 && (s[j-1] == 'l' || s[j-1] == 'L')) {
                        ++cnt; --j; if (cnt == 2) break;
                    }
                    longCount = cnt;
                    continue;
                }
                break;
            }

            std::string digits = s.substr(0, j);
            IntegerInfo info;
            try {
                if (digits.size() >= 2 && (digits[0] == '0') && (digits[1] == 'x' || digits[1] == 'X')) {
                    info.base = IntegerInfo::Base::Hexadecimal;
                    std::string body = digits.substr(2);
                    info.value = std::stoull(body, nullptr, 16);
                } else if (digits.size() >= 1 && digits[0] == '0' && digits.size() > 1) {
                    info.base = IntegerInfo::Base::Octal;
                    std::string body = digits.substr(1);
                    if (body.empty()) info.value = 0; else info.value = std::stoull(body, nullptr, 8);
                } else {
                    info.base = IntegerInfo::Base::Decimal;
                    info.value = std::stoull(digits, nullptr, 10);
                }
            } catch (...) {
                // Fallback: zero value on parse error
                info.value = 0;
                if (!digits.empty() && digits[0] == '0') info.base = IntegerInfo::Base::Octal; else info.base = IntegerInfo::Base::Decimal;
            }
                info.flags = IntegerInfo::FLAG_NONE;
                if (isUnsigned) info.flags |= IntegerInfo::FLAG_UNSIGNED;
                if (longCount == 1) info.flags |= IntegerInfo::FLAG_LONG;
                if (longCount == 2) info.flags |= IntegerInfo::FLAG_LONG_LONG;
            info.lexeme = s;
            return Token(IntegerToken{info}, pp.span);
        }
        case K::CharConst:
            return Token(CharacterToken{pp.lexeme}, pp.span);
        case K::StringLiteral:
            return Token(StringLiteralToken{pp.lexeme}, pp.span);
        case K::Punctuator:
            return Token(PunctuatorToken{pp.lexeme}, pp.span);
        default:
            return Token(EndOfFileToken{}, pp.span);
    }
}

Lexer::Lexer(wvmcc::Preprocessor& ps) : pp(&ps) { refillLA(); }

std::optional<Token> Lexer::peek() {
    skipWhitespaceAndNewlines();
    if (!la) return std::nullopt;
    return classify_local(*la);
}

std::optional<Token> Lexer::next() {
    skipWhitespaceAndNewlines();
    if (!pp) return std::nullopt;
    auto ppTok = pp->next();
    la = std::nullopt;
    refillLA();
    if (!ppTok) return std::nullopt;
    auto tk = classify_local(*ppTok);
    if (tk.kind() == TokenKind::EndOfFile) return std::nullopt;
    return tk;
}

void Lexer::skipWhitespaceAndNewlines() {
    refillLA();
    while (la) {
        if (la->kind == wvmcc::PPTokenKind::Whitespace || la->kind == wvmcc::PPTokenKind::Newline) {
            if (pp) pp->next();
            la = std::nullopt;
            refillLA();
            continue;
        }
        break;
    }
}

bool Lexer::consumeIfPunctuator(const std::string& punct) {
    skipWhitespaceAndNewlines();
    refillLA();
    if (!la) return false;
    if (la->kind == wvmcc::PPTokenKind::Punctuator && la->lexeme == punct) {
        if (pp) pp->next();
        la = std::nullopt;
        refillLA();
        return true;
    }
    return false;
}

std::optional<std::pair<std::string, wvmcc::SourceSpan>> Lexer::consumeIfIdentifier() {
    skipWhitespaceAndNewlines();
    refillLA();
    if (!la) return std::nullopt;
    if (la->kind == wvmcc::PPTokenKind::Identifier) {
        auto lex = la->lexeme;
        auto sp = la->span;
        if (pp) pp->next();
        la = std::nullopt;
        refillLA();
        return std::make_pair(lex, sp);
    }
    return std::nullopt;
}

bool Lexer::expectIdentifier(std::string& out, wvmcc::SourceSpan* span) {
    skipWhitespaceAndNewlines();
    refillLA();
    if (!la) return false;
    if (la->kind == wvmcc::PPTokenKind::Identifier) {
        out = la->lexeme;
        if (span) *span = la->span;
        if (pp) pp->next();
        la = std::nullopt;
        refillLA();
        return true;
    }
    return false;
}

ExprPtr Lexer::consumeIdentifierAsExpr() {
    auto opt = consumeIfIdentifier();
    if (!opt) return nullptr;
    auto node = std::make_shared<IdentifierExpr>();
    node->name = opt->first;
    node->span = opt->second;
    return node;
}

void Lexer::refillLA() {
    if (!la) {
        if (pp) la = pp->peek();
    }
}

} // namespace wvmcc::parser
