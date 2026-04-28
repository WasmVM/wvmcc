// Lexer: adapter between streaming Preprocessor and parser Token stream
#pragma once

#include "../pp/Preprocessor.hpp"
#include "Token.hpp"
#include "AST.hpp"
#include <optional>

namespace wvmcc::parser {

// Lexer emits parser `Token`s from the streaming `Preprocessor`.

class Lexer {
public:
    explicit Lexer(wvmcc::Preprocessor& ps);

    // Peek next parser Token (skips whitespace/newline)
    std::optional<Token> peek();

    // Consume and return next Token
    std::optional<Token> next();

    // Skip contiguous whitespace and newline tokens
    void skipWhitespaceAndNewlines();

    // If next token is a punctuator with exact lexeme `punct`, consume and return true
    bool consumeIfPunctuator(const std::string& punct);

    // If next token is an identifier, consume it and return its lexeme and span
    std::optional<std::pair<std::string, wvmcc::SourceSpan>> consumeIfIdentifier();

    // Expect an identifier; return true and fill `out` on success
    bool expectIdentifier(std::string& out, wvmcc::SourceSpan* span = nullptr);

    // Construct a parser Identifier expression node if next token is an identifier
    // Returns nullptr if not an identifier.
    ExprPtr consumeIdentifierAsExpr();

private:
    wvmcc::Preprocessor* pp{nullptr};
    std::optional<wvmcc::PPToken> la;

    void refillLA();
};

} // namespace wvmcc::parser
