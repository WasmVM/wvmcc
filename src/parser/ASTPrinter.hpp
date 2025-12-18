// Simple AST -> XML printer for diagnosis
#pragma once

#include <ostream>
#include <memory>
#include <string>
#include "AST.hpp"

namespace wvmcc::parser {

class ASTPrinter {
public:
    explicit ASTPrinter(std::ostream &os);
    void print(const TranslationUnitPtr &tu);

private:
    std::ostream &os_;
    int indent_ = 0;

    void writeIndent();
    void openTag(const std::string &name, const std::string &attrs = "");
    void closeTag(const std::string &name);
    void simpleTag(const std::string &name, const std::string &content);
    std::string esc(const std::string &s);

    // visitors
    void visitExternalDecl(const ExternalDeclPtr &d);
    void visitFunctionDef(const FunctionDefPtr &f);
    void visitDeclaration(const DeclarationPtr &d);
    void visitDeclarator(const DeclaratorPtr &d);
    void visitTypeNode(const TypeNodePtr &t);
    void visitBlockItem(const BlockItemPtr &b);
    void visitStmt(const StmtPtr &s);
    void visitExpr(const ExprPtr &e);
    // emit textual specifier entries for printing (replaces DeclarationSpecifiers::to_vector)
    void emitSpecifiersEntries(const DeclarationSpecifiers &specs);
};

} // namespace wvmcc::parser
