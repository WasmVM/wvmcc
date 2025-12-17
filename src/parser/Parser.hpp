// Minimal recursive-descent parser to build TranslationUnit from tokens
#pragma once

#include "AST.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include <vector>
#include <optional>

namespace wvmcc::parser {

class Parser {
public:
    explicit Parser(Lexer &lexer);
    TranslationUnitPtr parseTranslationUnit();

private:
    Lexer &lex;

    
    bool acceptPunct(const std::string &p);
    bool acceptKeyword(const std::string &k);

    
    ExternalDeclPtr parseExternalDecl();
    FunctionDefPtr parseFunctionDef(const std::vector<std::string>& specs, const std::string &name);
    DeclarationPtr parseDeclaration(const std::vector<std::string>& specs, const std::string &name);
    DeclaratorPtr makeSimpleDeclarator(const Token &t);

    // statements/expressions (very small subset)
    std::vector<BlockItemPtr> parseCompoundBody();
    StmtPtr parseStmt();
    ExprPtr parseExpr();
};

} // namespace wvmcc::parser
