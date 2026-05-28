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
    // track internal linkage (static) definitions by name -> (span, has_definitive_definition)
    std::unordered_map<std::string, std::pair<SourceSpan, bool>> internal_definitions{};
    // known typedef names (updated when parsing typedef declarations)
    std::unordered_set<std::string> typedef_names{};
    // tag registry for struct/union names -> specifier (incomplete or complete)
    std::unordered_map<std::string, std::shared_ptr<StructOrUnionSpecifier>> tag_registry{};
    // tag registry for enum names -> specifier (incomplete or complete)
    std::unordered_map<std::string, std::shared_ptr<DeclarationSpecifiers::TypeSpecifier::EnumSpecifier>> enum_tag_registry{};
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
