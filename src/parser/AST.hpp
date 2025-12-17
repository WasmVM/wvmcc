// Lightweight AST node definitions for the parser (C17 M0 subset)
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include "../common.hpp"
#include "Token.hpp"

namespace wvmcc::parser {

using wvmcc::SourcePos;
using wvmcc::SourceSpan;

struct Node {
    SourceSpan span;
    virtual ~Node() = default;
};

using NodePtr = std::shared_ptr<Node>;

// Forward declarations
struct TranslationUnit;
struct ExternalDecl;
struct FunctionDef;
struct Declaration;
struct Declarator;
struct TypeNode;
struct Expr;
struct Stmt;
struct BlockItem;

using TranslationUnitPtr = std::shared_ptr<TranslationUnit>;
using ExternalDeclPtr      = std::shared_ptr<ExternalDecl>;
using FunctionDefPtr      = std::shared_ptr<FunctionDef>;
using DeclarationPtr      = std::shared_ptr<Declaration>;
using DeclaratorPtr       = std::shared_ptr<Declarator>;
using TypeNodePtr         = std::shared_ptr<TypeNode>;
using ExprPtr             = std::shared_ptr<Expr>;
using StmtPtr             = std::shared_ptr<Stmt>;
using BlockItemPtr        = std::shared_ptr<BlockItem>;

// Translation unit: list of external declarations
struct TranslationUnit : Node {
    std::vector<ExternalDeclPtr> externals;
};

// ExternalDecl is either a function definition or a declaration
struct ExternalDecl : Node {
    std::variant<FunctionDefPtr, DeclarationPtr> decl;
};

// Basic type node (placeholder, to be expanded by semantic phase)
struct TypeNode : Node {
    enum class Kind { Builtin, Pointer, Array, Function, Struct, Union, Enum, Qualified } kind;
    // A human-readable representation for early stages
    std::string repr;
};

// Declarators (identifier, pointer, array, function)
struct Declarator : Node {
    struct Id { std::string name; } id;
    // nested declarator form (for pointers/arrays/functions)
    std::optional<DeclaratorPtr> inner;
};

struct Parameter {
    std::optional<std::string> typeSpec; // e.g., "int", "struct foo"
    DeclaratorPtr declarator;
    std::optional<ExprPtr> defaultValue;
};

// Declaration: specifiers + declarator + optional initializer
struct Declaration : Node {
    std::vector<std::string> specifiers; // storage-class, type-specifiers, qualifiers
    DeclaratorPtr declarator;
    std::optional<ExprPtr> initializer;
};

// Function definition: specifiers + declarator + params + body
struct FunctionDef : Node {
    std::vector<std::string> specifiers;
    DeclaratorPtr declarator; // name and type
    std::vector<Parameter> params;
    std::vector<BlockItemPtr> body; // compound-stmt flattened for now
};

// Expressions
struct Expr : Node {
    enum class Kind { Ident, Integer, Float, Char, String, Unary, Binary, Ternary, Call, Member, Index, Cast, Sizeof } kind;
};

struct IdentifierExpr : Expr { std::string name; };
struct IntegerLiteral : Expr { std::int64_t value; std::string raw; };
struct StringLiteral : Expr { std::string value; };

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr rhs;
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr lhs;
    ExprPtr rhs;
};

struct TernaryExpr : Expr {
    ExprPtr cond;
    ExprPtr thenExpr;
    ExprPtr elseExpr;
};

struct CallExpr : Expr { ExprPtr callee; std::vector<ExprPtr> args; };

struct MemberExpr : Expr { ExprPtr base; std::string member; bool isArrow = false; };

struct IndexExpr : Expr { ExprPtr base; ExprPtr index; };

// Statements and block items
struct Stmt : Node {
    enum class Kind { Expr, Compound, If, Switch, Case, Default, While, DoWhile, For, Return, Break, Continue, Goto, Label, Empty } kind;
};

struct ExprStmt : Stmt { ExprPtr expr; };

struct CompoundStmt : Stmt { std::vector<BlockItemPtr> items; };

struct IfStmt : Stmt { ExprPtr cond; StmtPtr thenStmt; std::optional<StmtPtr> elseStmt; };

struct WhileStmt : Stmt { ExprPtr cond; StmtPtr body; };

struct ForStmt : Stmt { std::optional<BlockItemPtr> init; std::optional<ExprPtr> cond; std::optional<ExprPtr> step; StmtPtr body; };

struct ReturnStmt : Stmt { std::optional<ExprPtr> value; };

struct BlockItem : Node {
    // either a declaration or a statement
    std::variant<DeclarationPtr, StmtPtr> item;
};

// Simple symbol placeholder to be filled by semantic phase
struct SymbolRef {
    std::string name;
    int id = -1;
};

} // namespace wvmcc::parser

// Small factory helpers to reduce repetitive std::make_shared<T>() usage
// Usage:
//   auto id = make_ast<IdentifierExpr>();
//   auto lit = make_ast<IntegerLiteral>();
//   auto node_with_span = make_ast_with_span<FunctionDef>(span);
namespace wvmcc::parser {

template<typename T, typename... Args>
inline std::shared_ptr<T> make_ast(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

inline std::shared_ptr<Node> make_node() {
    return std::make_shared<Node>();
}

// Create an AST node and set its SourceSpan in one call
template<typename T>
inline std::shared_ptr<T> make_ast_with_span(const SourceSpan &span) {
    auto p = std::make_shared<T>();
    p->span = span;
    return p;
}

} // namespace wvmcc::parser
