#ifndef WVMCC_AST_Expr_DEF
#define WVMCC_AST_Expr_DEF

#include <variant>
#include <optional>
#include <Token.hpp>
#include <Lexer.hpp>

namespace WasmVM {
namespace AST {

// struct AssignmentExpr;
// struct GenericSelection {

// };

// struct Expression;
struct PrimaryExpr : public std::variant<
    TokenType::Identifier,
    TokenType::StringLiteral,
    TokenType::IntegerConstant,
    TokenType::CharacterConstant,
    TokenType::FloatingConstant
    // Expression,
    // GenericSelection
>
{
    using Base = std::variant<
        TokenType::Identifier,
        TokenType::StringLiteral,
        TokenType::IntegerConstant,
        TokenType::CharacterConstant,
        TokenType::FloatingConstant
        // TODO: Expression,
        // TODO: GenericSelection
    >;

    template<typename T>
    PrimaryExpr(T value);
    
    static std::optional<PrimaryExpr> parse(Lexer& lexer);
};

struct PostfixExpr {
    enum class Type {
        subscript, member_of_obj, member_of_ptr, increment, decrement, func_call, compound
    };

    Type type;

    template<Type T>
    PostfixExpr();
    template<Type T>
    PostfixExpr(TokenType::Identifier);
    // TODO: Expression
    // TODO: argument-expression-list
    // TODO: typename initializer-list

    static std::optional<PostfixExpr> parse(Lexer& lexer);
};

// struct Expression {
//     static Expression parse(Lexer& lexer);
// };

} // AST
} // WasmVM

#endif