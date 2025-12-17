// Minimal recursive-descent parser to build TranslationUnit from tokens
#pragma once

#include "AST.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include "../pp/Diagnostics.hpp"
#include <optional>

namespace wvmcc::parser {

class Parser {
public:
    explicit Parser(Lexer &lexer);
    TranslationUnitPtr parseTranslationUnit();
    const std::vector<wvmcc::Diagnostic>& getDiagnostics() const { return diagnostics; }

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
    
private:
    std::vector<wvmcc::Diagnostic> diagnostics{};
    // track internal linkage (static) definitions by name -> (span, has_definitive_definition)
    std::unordered_map<std::string, std::pair<SourceSpan, bool>> internal_definitions{};
};

} // namespace wvmcc::parser
