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
using PrimaryExpr_Base = std::variant<
    TokenType::Identifier,
    TokenType::StringLiteral,
    TokenType::IntegerConstant,
    TokenType::CharacterConstant,
    TokenType::FloatingConstant
    // Expression,
    // GenericSelection
>;
struct PrimaryExpr : public PrimaryExpr_Base
{
    PrimaryExpr(TokenType::Identifier value) : PrimaryExpr_Base(value) {}
    PrimaryExpr(TokenType::StringLiteral value) : PrimaryExpr_Base(value) {}
    PrimaryExpr(TokenType::IntegerConstant value) : PrimaryExpr_Base(value) {}
    PrimaryExpr(TokenType::FloatingConstant value) : PrimaryExpr_Base(value) {}
    PrimaryExpr(TokenType::CharacterConstant value) : PrimaryExpr_Base(value) {}
    static std::optional<PrimaryExpr> parse(Lexer& lexer);
};

// struct Expression {
//     static Expression parse(Lexer& lexer);
// };

} // AST
} // WasmVM

#endif