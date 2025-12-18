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
struct StructOrUnionSpecifier;
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

// Bitmask-based specifier flags for compact representation and fast tests
enum class StorageClass : uint32_t { None = 0, Typedef = 1u<<0, Extern = 1u<<1, Static = 1u<<2, Auto = 1u<<3, Register = 1u<<4, ThreadLocal = 1u<<5 };
enum class TypeQualifier : uint32_t { None = 0, Const = 1u<<0, Volatile = 1u<<1, Restrict = 1u<<2, Atomic = 1u<<3 };
enum class FunctionSpecifier : uint32_t { None = 0, Inline = 1u<<0, NoReturn = 1u<<1 };

inline StorageClass operator|(StorageClass a, StorageClass b) { return static_cast<StorageClass>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
inline StorageClass& operator|=(StorageClass &a, StorageClass b) { a = a | b; return a; }
inline bool hasStorage(StorageClass flags, StorageClass bit) { return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(bit)) != 0; }

inline TypeQualifier operator|(TypeQualifier a, TypeQualifier b) { return static_cast<TypeQualifier>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
inline TypeQualifier& operator|=(TypeQualifier &a, TypeQualifier b) { a = a | b; return a; }
inline bool hasTypeQual(TypeQualifier flags, TypeQualifier bit) { return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(bit)) != 0; }

inline FunctionSpecifier operator|(FunctionSpecifier a, FunctionSpecifier b) { return static_cast<FunctionSpecifier>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
inline FunctionSpecifier& operator|=(FunctionSpecifier &a, FunctionSpecifier b) { a = a | b; return a; }
inline bool hasFuncSpec(FunctionSpecifier flags, FunctionSpecifier bit) { return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(bit)) != 0; }

struct DeclarationSpecifiers {
    StorageClass storageFlags{StorageClass::None};
    // Simple type-specifier tokens (keywords like 'int', 'long', 'unsigned')
    enum class SimpleTypeSpecifier : uint32_t {
        None = 0,
        Void,
        Char,
        Short,
        Int,
        Long,
        Float,
        Double,
        Signed,
        Unsigned,
        Bool,
        Complex,
        Imaginary
    };
    // Aggregate representation for type-specifiers (simple keyword sequences,
    // struct/union specifiers, typedef-names, or other combined forms).
    struct TypeSpecifier {
        enum class Kind { Simple, StructOrUnion, TypedefName, Other } kind{Kind::Other};
        std::vector<SimpleTypeSpecifier> simple; // used when Kind==Simple
        std::shared_ptr<StructOrUnionSpecifier> su; // used when Kind==StructOrUnion
        std::string text; // textual fallback (typedef-name or other combined form)
    };

    std::vector<TypeSpecifier> typeSpecifiers;
    TypeQualifier typeQualFlags{TypeQualifier::None};
    FunctionSpecifier funcSpecFlags{FunctionSpecifier::None};
    std::vector<std::string> alignSpec; // alignment specifiers may contain expressions

    bool empty() const {
        return storageFlags == StorageClass::None && typeSpecifiers.empty() && typeQualFlags == TypeQualifier::None && funcSpecFlags == FunctionSpecifier::None && alignSpec.empty();
    }

    void addStorage(StorageClass s) { storageFlags |= s; }
    void addTypeQual(TypeQualifier q) { typeQualFlags |= q; }
    void addFuncSpec(FunctionSpecifier f) { funcSpecFlags |= f; }

    bool hasStorage(StorageClass s) const { return wvmcc::parser::hasStorage(storageFlags, s); }
    bool hasTypeQual(TypeQualifier q) const { return wvmcc::parser::hasTypeQual(typeQualFlags, q); }
    bool hasFuncSpec(FunctionSpecifier f) const { return wvmcc::parser::hasFuncSpec(funcSpecFlags, f); }

    // Note: formatting/printing helpers moved to ASTPrinter.
};

// Declarator with optional bit-field width used inside struct/union members
struct StructDeclarator {
    DeclaratorPtr declarator; // may be null for anonymous bit-field or unnamed member
    std::optional<ExprPtr> bitfieldWidth;
};

// A struct/union member (a declaration with possibly multiple declarators)
struct StructMember {
    DeclarationSpecifiers specifiers;
    std::vector<StructDeclarator> declarators; // empty means unnamed/anonymous declaration
};

// Minimal holder for struct-or-union specifiers (name + whether body present).
struct StructOrUnionSpecifier {
    enum class Kind { Struct, Union } kind{Kind::Struct};
    std::optional<std::string> name;
    bool hasBody{false};
    std::vector<StructMember> members;
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
    DeclarationSpecifiers specifiers; // storage-class, type-specifiers, qualifiers
    DeclaratorPtr declarator;
    std::optional<ExprPtr> initializer;
};

// Function definition: specifiers + declarator + params + body
struct FunctionDef : Node {
    DeclarationSpecifiers specifiers;
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
