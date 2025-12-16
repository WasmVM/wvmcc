// Adapter between preprocessor Tokenizer and parser components.
#pragma once

#include "../pp/Tokenizer.hpp"
#include "../pp/Preprocessor.hpp"
#include "AST.hpp"
#include <optional>

namespace wvmcc::parser {

class PPBridge {
public:
    explicit PPBridge(wvmcc::Tokenizer& tz) : tz(&tz), pp(nullptr) { refillLA(); }

    explicit PPBridge(wvmcc::Preprocessor& ps) : tz(nullptr), pp(&ps) { refillLA(); }

    // Peek next PPToken without consuming
    std::optional<wvmcc::PPToken> peek() { refillLA(); return la; }

    // Consume and return next PPToken
    std::optional<wvmcc::PPToken> next() {
        std::optional<wvmcc::PPToken> tok;
        if (tz) tok = tz->next();
        else if (pp) tok = pp->next();
        la = std::nullopt;
        refillLA();
        return tok;
    }

    // Skip contiguous whitespace and newline tokens
    void skipWhitespaceAndNewlines() {
        refillLA();
        while (la) {
            if (la->kind == wvmcc::PPTokenKind::Whitespace || la->kind == wvmcc::PPTokenKind::Newline) {
                if (tz) tz->next(); else if (pp) pp->next();
                la = std::nullopt;
                refillLA();
                continue;
            }
            break;
        }
    }

    // If next token is a punctuator with exact lexeme `punct`, consume and return true
    bool consumeIfPunctuator(const std::string& punct) {
        refillLA();
        if (!la) return false;
        if (la->kind == wvmcc::PPTokenKind::Punctuator && la->lexeme == punct) {
            if (tz) tz->next(); else if (pp) pp->next();
            la = std::nullopt;
            refillLA();
            return true;
        }
        return false;
    }

    // If next token is an identifier, consume it and return its lexeme and span
    std::optional<std::pair<std::string, wvmcc::SourceSpan>> consumeIfIdentifier() {
        refillLA();
        if (!la) return std::nullopt;
        if (la->kind == wvmcc::PPTokenKind::Identifier) {
            auto lex = la->lexeme;
            auto sp = la->span;
            if (tz) tz->next(); else if (pp) pp->next();
            la = std::nullopt;
            refillLA();
            return std::make_pair(lex, sp);
        }
        return std::nullopt;
    }

    // Expect an identifier; return true and fill `out` on success
    bool expectIdentifier(std::string& out, wvmcc::SourceSpan* span = nullptr) {
        refillLA();
        if (!la) return false;
        if (la->kind == wvmcc::PPTokenKind::Identifier) {
            out = la->lexeme;
            if (span) *span = la->span;
            if (tz) tz->next(); else if (pp) pp->next();
            la = std::nullopt;
            refillLA();
            return true;
        }
        return false;
    }

    // Construct a parser Identifier expression node if next token is an identifier
    // Returns nullptr if not an identifier.
    ExprPtr consumeIdentifierAsExpr() {
        auto opt = consumeIfIdentifier();
        if (!opt) return nullptr;
        auto node = std::make_shared<IdentifierExpr>();
        node->name = opt->first;
        node->span = opt->second;
        return node;
    }

private:
    wvmcc::Tokenizer* tz{nullptr};
    wvmcc::Preprocessor* pp{nullptr};
    std::optional<wvmcc::PPToken> la;

    void refillLA() {
        if (!la) {
            if (tz) la = tz->peek();
            else if (pp) la = pp->peek();
        }
    }
};

} // namespace wvmcc::parser
