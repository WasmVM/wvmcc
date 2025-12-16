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
            if (keywords.count(pp.lexeme)) return Token(TokenKind::Keyword, pp.lexeme, pp.span);
            return Token(TokenKind::Identifier, pp.lexeme, pp.span);
        case K::PPNumber:
        case K::CharConst:
            return Token(TokenKind::Constant, pp.lexeme, pp.span);
        case K::StringLiteral:
            return Token(TokenKind::StringLiteral, pp.lexeme, pp.span);
        case K::Punctuator:
            return Token(TokenKind::Punctuator, pp.lexeme, pp.span);
        default:
            return Token(TokenKind::EndOfFile, pp.lexeme, pp.span);
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
    if (tk.kind == TokenKind::EndOfFile) return std::nullopt;
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
