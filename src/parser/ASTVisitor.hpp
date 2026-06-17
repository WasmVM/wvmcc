#pragma once

#include "AST.hpp"

namespace wvmcc::parser {

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    // convenience aliases for node pointer types
    using IdentifierExprPtr = std::shared_ptr<IdentifierExpr>;

    // Entry point
    void traverseTranslationUnit(const TranslationUnitPtr &tu);

    void traverseExternalDecl(const ExternalDeclPtr &ext);

    void traverseFunction(const FunctionDefPtr &f);

    void traverseDeclaration(const DeclarationPtr &d);

    void traverseInit(const InitializerPtr &in);

    void traverseStmt(const StmtPtr &s);

    void traverseExpr(const ExprPtr &e);

    // Report a full (top-level) expression: invokes onExpr once, then recurses.
    void traverseFullExpr(const ExprPtr &e);

protected:
    // callback hooks
    virtual void onIdent(const IdentifierExprPtr &/*id*/) {}
    virtual void onFunctionDef(const FunctionDefPtr &/*f*/) {}
    // static assertion hook: called for `_Static_assert` externals
    virtual void onStaticAssert(const ExternalDecl::StaticAssertPtr &/*sa*/) {}
    // called when entering/exiting a function body
    virtual void onEnterFunction(const FunctionDefPtr &/*f*/) {}
    virtual void onExitFunction(const FunctionDefPtr &/*f*/) {}
    virtual void onDeclaration(const DeclarationPtr &/*d*/) {}
    // Called once for each *full* expression encountered in an evaluated
    // context (expression statement, controlling expression, return value,
    // initializer expression, etc.). Subexpressions are NOT reported here —
    // the hook receives the top of the expression tree so a consumer can run a
    // single recursive type/constraint analysis per full expression without
    // duplicating diagnostics. Default: no-op.
    virtual void onExpr(const ExprPtr &/*e*/) {}
    // Called once for each statement as traverseStmt visits it (before its
    // sub-statements/expressions), so a consumer can apply statement-level
    // constraints (controlling-expression type, for-clause storage class).
    // Default: no-op.
    virtual void onStmt(const StmtPtr &/*s*/) {}
    // Called when entering/exiting a block (compound-statement) scope. Lets a
    // consumer track block-scope symbol tables for redefinition diagnostics.
    virtual void onEnterBlock() {}
    virtual void onExitBlock() {}
};

} // namespace wvmcc::parser
