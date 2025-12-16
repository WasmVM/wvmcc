// Parser-facing token type (language tokens after preprocessing)
#pragma once

#include <string>
#include <variant>
#include <optional>
#include "../common.hpp"
#include <optional>

namespace wvmcc::parser {

enum class TokenKind {
    Keyword,
    Identifier,
    Constant,        // numeric or character
    StringLiteral,
    Punctuator,
    EndOfFile
};

struct Token {
    TokenKind kind;
    std::string lexeme;    // exact source lexeme
    wvmcc::SourceSpan span;

    // For punctuators we keep the lexeme (e.g., ";", "->", "==")
    Token(TokenKind k, std::string lex, const wvmcc::SourceSpan& s)
        : kind(k), lexeme(std::move(lex)), span(s) {}
};

} // namespace wvmcc::parser
