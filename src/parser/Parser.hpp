// Minimal recursive-descent parser to build TranslationUnit from tokens
#pragma once

#include "AST.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include "../common.hpp"
#include <optional>

namespace wvmcc::parser {

class Parser {
public:
    explicit Parser(Lexer &lexer);
    TranslationUnitPtr parseTranslationUnit();
    const std::vector<wvmcc::Diagnostic>& getDiagnostics() const { return diagnostics; }
    // Non-const access for passes (semantic checks) to append diagnostics
    std::vector<wvmcc::Diagnostic>& getDiagnosticsRef() { return diagnostics; }

private:
    Lexer &lex;

    
    bool acceptPunct(const std::string &p);
    bool acceptKeyword(const std::string &k);

    // collect consecutive keyword specifiers (e.g., 'static', 'int')
    // DeclarationSpecifiers is defined in AST.hpp and reused here
    DeclarationSpecifiers parseDeclarationSpecifiers();
    // parse a struct/union/enum specifier including member list when present
    DeclarationSpecifiers::TypeSpecifier parseStructOrUnionSpecifier();
    // parse an enum specifier including enumerator list when present
    DeclarationSpecifiers::TypeSpecifier parseEnumSpecifier();
    // parse the list of struct-declarations inside a struct/union body
    std::vector<StructMember> parseStructDeclarationList();
    // parse a single struct-declarator (optional declarator and optional bit-field width)
    StructDeclarator parseStructDeclarator();

    // parse declarators (identifier, pointer, array, function)
    DeclaratorPtr parseDeclarator();
    // parse one or more `__attribute__((...))` specifiers and return a flat
    // list of attributes. Returns an empty vector if no attribute specifier
    // follows. Unknown attribute names are kept; unsupported argument forms
    // are skipped.
    std::vector<GnuAttribute> parseGnuAttributeSpecifierList();
    // initializer parsing (assignment-expression or braced initializer-list)
    InitializerPtr parseInitializer();
    Designator parseDesignator();

    
    ExternalDeclPtr parseExternalDecl();
    FunctionDefPtr parseFunctionDef(const DeclarationSpecifiers& specs, const DeclaratorPtr &decl);
    DeclarationPtr parseDeclaration(const DeclarationSpecifiers& specs, const std::string &name);
    DeclarationPtr parseDeclaration(const DeclarationSpecifiers& specs, const DeclaratorPtr &decl);
    // Parse a full init-declarator-list (`d1 [= i1], d2 [= i2], …;`) given the
    // already-parsed first declarator. Returns one single-declarator
    // Declaration per init-declarator so downstream consumers stay unchanged.
    // Consumes the terminating ';'.
    std::vector<DeclarationPtr> parseInitDeclaratorList(const DeclarationSpecifiers& specs,
                                                        const DeclaratorPtr &first);

    // statements/expressions (very small subset)
    std::vector<BlockItemPtr> parseCompoundBody();
    StmtPtr parseStmt();
    ExprPtr parseAssignmentExpression();
    // C 'expression' includes the comma operator (lowest precedence)
    ExprPtr parseExpression();
    ExprPtr parsePrimary();
    ExprPtr parseUnaryExpression();
    ExprPtr parseConditionalExpression();
    ExprPtr parseLogicalOrExpression();
    ExprPtr parseLogicalAndExpression();
    ExprPtr parseInclusiveOrExpression();
    ExprPtr parseExclusiveOrExpression();
    ExprPtr parseAndExpression();
    ExprPtr parseEqualityExpression();
    ExprPtr parseRelationalExpression();
    ExprPtr parseShiftExpression();
    ExprPtr parseMultiplicativeExpression();
    ExprPtr parseAdditiveExpression();
    ExprPtr parseCastExpression();
    ExprPtr parsePostfixExpression();
    // Apply postfix operators to an already-parsed LHS (for label-vs-expr disambiguation).
    ExprPtr applyPostfixSuffix(ExprPtr lhs);
    
private:
    std::vector<wvmcc::Diagnostic> diagnostics{};
    // Extra file-scope declarations produced by a multi-declarator declaration
    // (`int a, b;`). parseExternalDecl returns the first and queues the rest
    // here; parseTranslationUnit drains this after each call.
    std::vector<ExternalDeclPtr> pendingExternals_{};
    // track internal linkage (static) definitions by name -> (span, has_definitive_definition)
    std::unordered_map<std::string, std::pair<SourceSpan, bool>> internal_definitions{};
    // known typedef names (updated when parsing typedef declarations)
    std::unordered_set<std::string> typedef_names{};
    // tag registry for struct/union names -> specifier (incomplete or complete)
    std::unordered_map<std::string, std::shared_ptr<StructOrUnionSpecifier>> tag_registry{};
    // tag registry for enum names -> specifier (incomplete or complete)
    std::unordered_map<std::string, std::shared_ptr<DeclarationSpecifiers::TypeSpecifier::EnumSpecifier>> enum_tag_registry{};
    // enumeration constant name -> value, so enum constants fold to integer
    // constants in constant expressions parsed before semantic analysis.
    std::unordered_map<std::string, long long> enum_constants{};
    // typedef-name -> underlying simple-type specifiers, but only for typedefs
    // that name a plain scalar type (e.g. `typedef unsigned long size_t;`).
    // Lets such typedef-names resolve to their builtin type in constant
    // expressions (sizeof/_Alignof/casts/_Generic) parsed before semantics.
    std::unordered_map<std::string, std::vector<DeclarationSpecifiers::SimpleTypeSpecifier>> typedef_simple{};
    // Record `name` as a typedef; if its declarator is a plain identifier and
    // its specifiers name a scalar type (directly or via another simple
    // typedef), remember the underlying simple specifiers in typedef_simple.
    void recordTypedef(const std::string &name, const DeclarationSpecifiers &specs, const DeclaratorPtr &declr);
    // Build a TypeNode for a type-name's base specifiers (Simple/struct/union/
    // enum/typedef-name) wrapped in `pointerDepth` pointer layers. Used by the
    // sizeof/_Alignof type-name forms to support abstract pointer declarators
    // (`sizeof(T *)`). Returns null if the base specifiers are empty.
    TypeNodePtr buildTypeNameNode(const DeclarationSpecifiers &specs, int pointerDepth,
                                  const std::vector<ExprPtr> &arrayDims = {});
    // Consume the pointer part of an abstract declarator (`*` `const`* …) and
    // return the pointer depth. Leaves any following tokens (e.g. `)`) in place.
    int parseAbstractPointerDepth();
    // Consume trailing array dimensions of an abstract declarator (`[N][M]…`),
    // appending each size expression (outermost first). Leaves other tokens.
    void parseAbstractArrayDims(std::vector<ExprPtr> &dims);
    // Block nesting depth (0 = file scope). Enum-constant folding is limited to
    // file scope, where the name cannot be shadowed by a local variable.
    int blockDepth{0};
    // labels seen in the current function (to enforce uniqueness)
    std::unordered_set<std::string> labels_in_current_function{};
    // gotos recorded in the current function (label name + span)
    std::vector<std::pair<std::string, SourceSpan>> gotos_in_current_function{};
    // current function's declaration specifiers while parsing body
    std::optional<DeclarationSpecifiers> current_function_specs{};
    // true if the current function's declarator wraps its return type in
    // one or more pointer layers (so a `void` simple specifier really means
    // `void *…` and bare `return expr;` is fine).
    bool current_function_returns_pointer{false};
    // context stack to track whether we're inside loops/switches for continue/break
    std::vector<Stmt::Kind> stmt_context_stack{};
};

} // namespace wvmcc::parser
