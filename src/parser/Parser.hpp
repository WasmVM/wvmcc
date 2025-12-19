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
    // parse a struct/union/enum specifier including member list when present
    DeclarationSpecifiers::TypeSpecifier parseStructOrUnionSpecifier();
    // parse the list of struct-declarations inside a struct/union body
    std::vector<StructMember> parseStructDeclarationList();
    // parse a single struct-declarator (optional declarator and optional bit-field width)
    StructDeclarator parseStructDeclarator();

    
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
    // tag registry for struct/union names -> specifier (incomplete or complete)
    std::unordered_map<std::string, std::shared_ptr<StructOrUnionSpecifier>> tag_registry{};
};

} // namespace wvmcc::parser
