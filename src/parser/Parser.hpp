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

    // collect consecutive keyword specifiers (e.g., 'static', 'int')
    // DeclarationSpecifiers is defined in AST.hpp and reused here
    DeclarationSpecifiers parseDeclarationSpecifiers();

    
    ExternalDeclPtr parseExternalDecl();
    FunctionDefPtr parseFunctionDef(const DeclarationSpecifiers& specs, const std::string &name);
    DeclarationPtr parseDeclaration(const DeclarationSpecifiers& specs, const std::string &name);

    // statements/expressions (very small subset)
    std::vector<BlockItemPtr> parseCompoundBody();
    StmtPtr parseStmt();
    ExprPtr parseExpr();
    
private:
    std::vector<wvmcc::Diagnostic> diagnostics{};
    // track internal linkage (static) definitions by name -> (span, has_definitive_definition)
    std::unordered_map<std::string, std::pair<SourceSpan, bool>> internal_definitions{};
    // known typedef names (updated when parsing typedef declarations)
    std::unordered_set<std::string> typedef_names{};
};

} // namespace wvmcc::parser
