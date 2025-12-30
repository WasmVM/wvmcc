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

protected:
    // callback hooks
    virtual void onIdent(const IdentifierExprPtr &/*id*/) {}
    virtual void onFunctionDef(const FunctionDefPtr &/*f*/) {}
    // called when entering/exiting a function body
    virtual void onEnterFunction(const FunctionDefPtr &/*f*/) {}
    virtual void onExitFunction(const FunctionDefPtr &/*f*/) {}
    virtual void onDeclaration(const DeclarationPtr &/*d*/) {}
};

} // namespace wvmcc::parser
