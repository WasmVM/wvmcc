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
struct Parameter;
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

// forward declare initializer-related nodes so Declaration can reference them
struct Initializer;
using InitializerPtr = std::shared_ptr<Initializer>;

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
        enum class Kind { Simple, StructOrUnion, Enum, TypedefName, Atomic, Other } kind{Kind::Other};
        std::vector<SimpleTypeSpecifier> simple; // used when Kind==Simple
        std::shared_ptr<StructOrUnionSpecifier> su; // used when Kind==StructOrUnion
        // enum specifier (used when Kind==Enum)
        struct EnumSpecifier;
        std::shared_ptr<EnumSpecifier> en;
        // atomic type wrapper: represents `_Atomic(type)` form. If present,
        // `atomicInner` holds the inner declaration-specifiers describing the
        // wrapped type.
        std::shared_ptr<DeclarationSpecifiers> atomicInner;
        std::string text; // textual fallback (typedef-name or other combined form)
    };

    std::vector<TypeSpecifier> typeSpecifiers;
    TypeQualifier typeQualFlags{TypeQualifier::None};
    FunctionSpecifier funcSpecFlags{FunctionSpecifier::None};
    std::vector<std::string> alignSpec; // alignment specifiers may contain expressions
    std::vector<ExprPtr> alignExprs; // parsed alignment expressions from _Alignas(...)

    bool empty() const {
        return storageFlags == StorageClass::None && typeSpecifiers.empty() && typeQualFlags == TypeQualifier::None && funcSpecFlags == FunctionSpecifier::None && alignSpec.empty() && alignExprs.empty();
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

// Minimal holder for enum specifiers (name + whether body present + enumerators).
struct DeclarationSpecifiers::TypeSpecifier::EnumSpecifier {
    std::optional<std::string> name;
    bool hasBody{false};
    struct Enumerator {
        std::string name;
        std::optional<ExprPtr> value; // optional constant-expression
    };
    std::vector<Enumerator> enumerators;
};

// ExternalDecl is either a function definition or a declaration
struct ExternalDecl : Node {
    // External declaration may be a function definition, a declaration, or
    // a static assertion (C 6.7.10). Static assertions are represented as
    // a small AST node so semantic checks can evaluate them with TU context.
    struct StaticAssert;
    using StaticAssertPtr = std::shared_ptr<StaticAssert>;

    std::variant<FunctionDefPtr, DeclarationPtr, StaticAssertPtr> decl;
};

// Static assertion node for `_Static_assert(constant-expression, string-literal);`
struct ExternalDecl::StaticAssert : Node {
    ExprPtr expr; // constant-expression
    ExprPtr message; // string-literal expression
};

// Basic type node (placeholder, to be expanded by semantic phase)
struct TypeNode : Node {
    enum class Kind { Builtin, Pointer, Array, Function, Struct, Union, Enum, Qualified } kind;
    // Structured type representation
    // Builtin: list of simple tokens (e.g., int, unsigned) or textual fallback
    std::vector<DeclarationSpecifiers::SimpleTypeSpecifier> simple;
    std::string text; // textual fallback (typedef-name or other)
    // Struct/Union/Enum
    std::shared_ptr<StructOrUnionSpecifier> su;

    // Pointer
    std::shared_ptr<TypeNode> pointee;
    TypeQualifier ptrQual{TypeQualifier::None};

    // Array
    std::shared_ptr<TypeNode> element;
    std::optional<ExprPtr> sizeExpr;
    bool arrayIsStatic{false};
    bool arrayIsStar{false};
    TypeQualifier arrayQual{TypeQualifier::None};

    // Function
    std::vector<std::shared_ptr<TypeNode>> params;
    bool hasParamTypeList{false};
    bool isVariadic{false};
    // For function type, returnType is represented by nesting: function -> pointee? use element/pointee as appropriate
};

// Defined before Declarator: Declarator::FunctionInfo holds
// std::vector<Parameter> by value, and Declarator's implicitly-defined
// (virtual, via Node) destructor is required at the end of the class
// definition — Parameter must be complete there (clang rejects the
// forward-declared form; see the GitHub CI clang builds).
struct Parameter {
    DeclarationSpecifiers specifiers;          // full type specifiers for this param
    std::optional<std::string> typeSpec;       // legacy string hint (may be empty)
    DeclaratorPtr declarator;
    std::optional<ExprPtr> defaultValue;
};

// Declarators (identifier, pointer, array, function)
struct Declarator : Node {
    struct Id { std::string name; } id;
    // nested declarator form (for pointers/arrays/functions)
    std::optional<DeclaratorPtr> inner;

    enum class Kind { Identifier, Pointer, Array, Function, Nested } kind{Kind::Identifier};

    // Pointer-specific info
    TypeQualifier ptrQual{TypeQualifier::None};

    // Array-specific info
    struct ArrayInfo {
        std::optional<ExprPtr> size; // optional assignment-expression
        bool isStatic{false};
        bool isStar{false}; // means '[ * ]'
        TypeQualifier qual{TypeQualifier::None};
    } array;

    // Function-specific info
    struct FunctionInfo {
        std::vector<Parameter> params; // parameter-type-list
        bool hasParamTypeList{false};
        bool isVariadic{false};        // trailing `...` present
        std::vector<std::string> identifierList; // identifier-list (old K&R style)
    } function;
};

// GCC-style attribute specifier: __attribute__((name, name(args...), ...))
// Unknown attribute names are silently retained so codegen / later passes can
// inspect them. String arguments are stored decoded (no surrounding quotes).
struct GnuAttribute {
    std::string name;
    std::vector<std::string> stringArgs;
    std::vector<long long> intArgs;   // integer-constant args, e.g. wvmcc_memidx(2)
};

// Declaration: specifiers + declarator + optional initializer
struct Declaration : Node {
    DeclarationSpecifiers specifiers; // storage-class, type-specifiers, qualifiers
    DeclaratorPtr declarator;
    std::optional<InitializerPtr> initializer;
    std::vector<GnuAttribute> gnuAttributes;
};

// Function definition: specifiers + declarator + params + body
struct FunctionDef : Node {
    DeclarationSpecifiers specifiers;
    DeclaratorPtr declarator; // name and type
    std::vector<Parameter> params;
    bool isVariadic{false}; // copied from declarator->function.isVariadic
    std::vector<BlockItemPtr> body; // compound-stmt flattened for now
    std::vector<GnuAttribute> gnuAttributes;
};

// Expressions
struct Expr : Node {
    enum class Kind { Ident, Integer, Float, Char, String, Unary, PostfixUnary, Binary, Ternary, Call, Member, Index, Cast, Sizeof, AlignOf, CompoundLiteral, GenericSelection } kind;
};

struct IdentifierExpr : Expr { std::string name; };
struct IntegerLiteral : Expr {
    std::int64_t value;
    std::string raw;
    // True when the literal has unsigned type (u/U suffix, or — per 6.4.4.1p5 —
    // its value exceeds the largest signed type that could hold it). Threaded
    // through to the constant-expression evaluator so comparisons / division /
    // shifts of full-width unsigned constants use unsigned semantics.
    bool isUnsigned{false};
    // Integer-conversion rank of the literal's resolved type (l/L → long,
    // ll/LL → long long, or a value too large for a narrower candidate). Carried
    // so type-sensitive constant contexts — notably `_Generic` selection — see
    // the literal's real type (`0L` is `long`, not `int`). 6.4.4.1p5.
    bool isLong{false};
    bool isLongLong{false};
};
struct StringLiteral : Expr { std::string value; };
struct CharLiteral : Expr { char value; };
struct FloatLiteral : Expr {
    double value;
    std::string raw;
    bool isFloat{false}; // true → f32 (suffix f/F); false → f64 (suffix L or none)
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr rhs;
};

struct PostfixUnaryExpr : Expr {
    enum class Op { Inc, Dec } op; // increment or decrement
    ExprPtr base;
};

struct CastExpr : Expr {
    TypeNodePtr type;
    ExprPtr expr;
};

struct SizeofExpr : Expr {
    std::optional<TypeNodePtr> type; // if present, sizeof(type)
    ExprPtr expr; // if type not present, sizeof expr
    // Raw type-specifiers for the `sizeof(type-name)` form. Preferred over
    // `type` by codegen: lets it resolve struct tags / typedef-names to the
    // complete type via Semantic::canonicalTypeRepr (the `type` field only
    // ever held a placeholder for the type-name form).
    std::optional<DeclarationSpecifiers> typeSpecs;
};

struct AlignOfExpr : Expr {
    TypeNodePtr type;
    std::string typeText; // textual representation of the type inside _Alignof
    // Raw type-specifiers for `_Alignof(type-name)` (see SizeofExpr::typeSpecs).
    std::optional<DeclarationSpecifiers> typeSpecs;
    // `_Alignof(expression)` operand (GNU/clang extension): when set, the
    // alignment is that of the operand's type, honoring any _Alignas on a
    // named object. Mutually exclusive with the type-name forms above.
    ExprPtr expr;
};

struct CompoundLiteral : Expr {
    TypeNodePtr type;
    InitializerPtr init;
};

struct GenericAssociation {
    bool isDefault{false};
    TypeNodePtr type; // null for default
    ExprPtr expr;
};

struct GenericSelectionExpr : Expr {
    ExprPtr controlling;
    std::vector<GenericAssociation> assocs;
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

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    // For __builtin_va_arg(ap, T): T is parsed as a type-name and stored here.
    // Null for all other calls.
    TypeNodePtr vaArgType;
};

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

struct DoWhileStmt : Stmt { StmtPtr body; ExprPtr cond; };

struct BreakStmt : Stmt {};

struct ContinueStmt : Stmt {};

struct GotoStmt : Stmt { std::string label; };

struct SwitchStmt : Stmt { ExprPtr cond; StmtPtr body; };

struct CaseStmt : Stmt { ExprPtr value; StmtPtr stmt; };

struct DefaultStmt : Stmt { StmtPtr stmt; };

struct LabelStmt : Stmt { std::string name; StmtPtr stmt; };

struct BlockItem : Node {
    // either a declaration, a statement, or a _Static_assert (C17 §6.8.2)
    std::variant<DeclarationPtr, StmtPtr, ExternalDecl::StaticAssertPtr> item;
};

// Designators used in designated initializers (e.g., [3], .member)
struct Designator {
    enum class Kind { Index, Member } kind{Kind::Index};
    std::optional<ExprPtr> index; // used when Kind==Index
    std::string member;           // used when Kind==Member
};

// An initializer clause: optional list of designators followed by an initializer
struct Initializer; // forward
using InitializerPtr = std::shared_ptr<Initializer>;

struct InitClause {
    std::vector<Designator> designators;
    InitializerPtr init; // nested initializer or expression
};

// Initializer: either an assignment-expression (Expr) or a braced initializer-list
struct Initializer : Node {
    enum class Kind { Expr, List } kind{Kind::Expr};
    ExprPtr expr; // valid when kind==Expr
    std::vector<InitClause> clauses; // valid when kind==List
    bool trailingComma{false};
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
