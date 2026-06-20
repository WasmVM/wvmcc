// Parser-facing token types (language tokens after preprocessing)
#pragma once

#include <string>
#include <variant>
#include <optional>
#include "../common.hpp"
#include <cstdint>

namespace wvmcc::parser {

enum class TokenKind {
    Keyword,
    Identifier,
    IntegerConstant,
    FloatingConstant,
    EnumerationConstant,
    CharacterConstant,
    StringLiteral,
    Punctuator,
    Other,
    EndOfFile
};

struct KeywordToken { std::string lexeme; };
struct IdentifierToken { std::string name; };
struct EnumerationToken { std::string name; };
struct CharacterInfo {
    enum class ResolvedType { UChar, WChar, Char16, Char32 } resolved{ResolvedType::UChar};
    std::uint32_t value{0};
    std::string lexeme;
};
struct CharacterToken { CharacterInfo info; };
struct StringLiteralToken { std::string lexeme; };
struct PunctuatorToken { std::string lexeme; };

struct IntegerInfo {
    enum class Base { Decimal, Octal, Hexadecimal } base;
    std::uint64_t value;
    // flags bitfield: can combine Unsigned and length modifiers
    enum Flags : uint32_t {
        FLAG_NONE = 0,
        FLAG_UNSIGNED = 1u << 0,
        FLAG_LONG = 1u << 1,
        FLAG_LONG_LONG = 1u << 2
    };
    uint32_t flags{FLAG_NONE};
    std::string lexeme;
    // Resolved semantic type for the integer literal
    enum class ResolvedType {
        Int,
        UnsignedInt,
        Long,
        UnsignedLong,
        LongLong,
        UnsignedLongLong,
        None
    } resolved{ResolvedType::None};
};

struct IntegerToken { IntegerInfo info; };
struct FloatingToken {
    std::string lexeme;
    enum class ResolvedType { Float, Double, LongDouble } resolved{ResolvedType::Double};
};
struct EndOfFileToken {};
// A stray, non-white-space character that cannot form any other token
// (C 6.4p1's final "each non-white-space character that cannot be one of the
// above"). Kept distinct from EndOfFile so the parser can diagnose it rather
// than mistake it for end-of-input.
struct OtherToken { std::string lexeme; };

using TokenVariant = std::variant<KeywordToken, IdentifierToken, IntegerToken, FloatingToken, EnumerationToken, CharacterToken, StringLiteralToken, PunctuatorToken, OtherToken, EndOfFileToken>;

struct Token {
    TokenVariant v;
    wvmcc::SourceSpan span;

    Token(TokenVariant vv, const wvmcc::SourceSpan& s) : v(std::move(vv)), span(s) {}

    TokenKind kind() const {
        return std::visit([](auto&& arg) -> TokenKind {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, KeywordToken>) return TokenKind::Keyword;
            if constexpr (std::is_same_v<T, IdentifierToken>) return TokenKind::Identifier;
            if constexpr (std::is_same_v<T, IntegerToken>) return TokenKind::IntegerConstant;
            if constexpr (std::is_same_v<T, FloatingToken>) return TokenKind::FloatingConstant;
            if constexpr (std::is_same_v<T, EnumerationToken>) return TokenKind::EnumerationConstant;
            if constexpr (std::is_same_v<T, CharacterToken>) return TokenKind::CharacterConstant;
            if constexpr (std::is_same_v<T, StringLiteralToken>) return TokenKind::StringLiteral;
            if constexpr (std::is_same_v<T, PunctuatorToken>) return TokenKind::Punctuator;
            if constexpr (std::is_same_v<T, OtherToken>) return TokenKind::Other;
            return TokenKind::EndOfFile;
        }, v);
    }

    std::string lexeme() const {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, KeywordToken>) return arg.lexeme;
            if constexpr (std::is_same_v<T, IdentifierToken>) return arg.name;
            if constexpr (std::is_same_v<T, IntegerToken>) return arg.info.lexeme;
            if constexpr (std::is_same_v<T, FloatingToken>) return arg.lexeme;
            if constexpr (std::is_same_v<T, EnumerationToken>) return arg.name;
            if constexpr (std::is_same_v<T, CharacterToken>) return arg.info.lexeme;
            if constexpr (std::is_same_v<T, StringLiteralToken>) return arg.lexeme;
            if constexpr (std::is_same_v<T, PunctuatorToken>) return arg.lexeme;
            if constexpr (std::is_same_v<T, OtherToken>) return arg.lexeme;
            return std::string();
        }, v);
    }
};

} // namespace wvmcc::parser
