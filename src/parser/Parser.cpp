// Very small parser implementation sufficient for basic translation unit parsing
#include "Parser.hpp"
#include <cassert>

namespace wvmcc::parser {

Parser::Parser(Lexer &lexer) : lex(lexer) {}

// NOTE: Parser no longer provides peek()/next() wrappers; use lex.peek()/lex.next() directly.

bool Parser::acceptPunct(const std::string &p) {
    auto t = lex.peek();
    if (t && t->kind() == TokenKind::Punctuator && t->lexeme() == p) {
        lex.next();
        return true;
    }
    return false;
}

bool Parser::acceptKeyword(const std::string &k) {
    auto t = lex.peek();
    if (t && t->kind() == TokenKind::Keyword && t->lexeme() == k) {
        lex.next();
        return true;
    }
    return false;
}

TranslationUnitPtr Parser::parseTranslationUnit() {
    auto tu = std::make_shared<TranslationUnit>();
    while (lex.peek() != std::nullopt) {
        // parse external declaration
        auto ext = parseExternalDecl();
        if (ext) tu->externals.push_back(ext);
        else break;
    }
    return tu;
}

ExternalDeclPtr Parser::parseExternalDecl() {
    // gather specifiers: accept keywords (like 'int') and identifiers that look like type-spec
    std::vector<std::string> specs;
    
    for (auto t = lex.peek(); 
        (t != std::nullopt) && (t->kind() == TokenKind::Keyword);
        t = lex.next()
    ) {
        specs.push_back(t->lexeme());
    }

    auto nameTok = lex.peek();
    if (!nameTok) return nullptr;

    // if next is identifier, consume as name
    std::string name;
    if (nameTok->kind() == TokenKind::Identifier) {
        name = nameTok->lexeme();
        lex.next();
    }

    // if followed by '(' => function (params ignored for now)
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") {
        // consume '(' and try to consume matching ')'
        lex.next(); // (
        // skip until matching ')'
        int depth = 1;
        while (auto p = lex.peek()) {
            if (p->kind() == TokenKind::Punctuator) {
                if (p->lexeme() == "(") ++depth;
                else if (p->lexeme() == ")") {
                    --depth;
                    lex.next();
                    if (depth == 0) break;
                    continue;
                }
            }
            lex.next();
        }

        // if next is '{' parse body
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
            auto f = parseFunctionDef(specs, name);
            if (!f) return nullptr;
            auto ext = std::make_shared<ExternalDecl>();
            ext->decl = f;
            ext->span = f->span;
            return ext;
        } else {
            // otherwise treat as declaration (prototype)
            auto d = parseDeclaration(specs, name);
            if (!d) return nullptr;
            auto ext = std::make_shared<ExternalDecl>();
            ext->decl = d;
            ext->span = d->span;
            return ext;
        }
    }

    // otherwise if next token sequence ends with ';' treat as declaration
    // consume until semicolon
    if (!name.empty()) {
        auto d = parseDeclaration(specs, name);
        if (!d) return nullptr;
        auto ext = std::make_shared<ExternalDecl>();
        ext->decl = d;
        ext->span = d->span;
        return ext;
    }

    // fallback: consume one token and wrap as external decl
    auto t = lex.next();
    if (!t) return nullptr;
    auto decl = std::make_shared<Declaration>();
    decl->span = t->span;
    decl->specifiers.push_back(t->lexeme());
    auto dtor = makeSimpleDeclarator(*t);
    decl->declarator = dtor;
    auto ext = std::make_shared<ExternalDecl>();
    ext->decl = decl;
    ext->span = t->span;
    return ext;
}

FunctionDefPtr Parser::parseFunctionDef(const std::vector<std::string>& specs, const std::string &name) {
    // we are at '(' already consumed and ) consumed by caller; next is '{'
    // consume '{'
    if (!acceptPunct("{")) return nullptr;
    auto f = std::make_shared<FunctionDef>();
    f->specifiers = specs;
    auto d = std::make_shared<Declarator>();
    d->id.name = name;
    f->declarator = d;
    f->body = parseCompoundBody();
    return f;
}

DeclarationPtr Parser::parseDeclaration(const std::vector<std::string>& specs, const std::string &name) {
    auto decl = std::make_shared<Declaration>();
    decl->specifiers = specs;
    if (!name.empty()) {
        auto d = std::make_shared<Declarator>();
        d->id.name = name;
        decl->declarator = d;
    }
    // consume until semicolon or EOF
    while (auto p = lex.peek()) {
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == ";") { lex.next(); break; }
        // simple initializer: '=' expr ;
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == "=") {
            lex.next();
            decl->initializer = parseExpr();
            // consume until ;
            while (auto q = lex.peek()) { if (q->kind()==TokenKind::Punctuator && q->lexeme()==";") { lex.next(); break; } lex.next(); }
            break;
        }
        lex.next();
    }
    return decl;
}

DeclaratorPtr Parser::makeSimpleDeclarator(const Token &t) {
    auto d = std::make_shared<Declarator>();
    d->span = t.span;
    d->id.name = t.lexeme();
    return d;
}

std::vector<BlockItemPtr> Parser::parseCompoundBody() {
    std::vector<BlockItemPtr> body;
    // we have consumed '{'
    while (auto p = lex.peek()) {
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == "}") { lex.next(); break; }
        // simple handling: if keyword 'return'
        if (p->kind() == TokenKind::Keyword && p->lexeme() == "return") {
            lex.next();
            auto rs = std::make_shared<ReturnStmt>();
            rs->span = p->span;
            rs->value = parseExpr();
            // consume trailing ;
            if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
            auto bi = std::make_shared<BlockItem>();
            bi->item = std::static_pointer_cast<Stmt>(rs);
            body.push_back(bi);
            continue;
        }

        // declaration start
        if (p->kind() == TokenKind::Keyword) {
            // simple declaration: keyword identifier = expr ;
            std::vector<std::string> specs;
            specs.push_back(p->lexeme()); lex.next();
            std::string name;
            if (lex.peek() && lex.peek()->kind()==TokenKind::Identifier) { name = lex.peek()->lexeme(); lex.next(); }
            auto decl = parseDeclaration(specs, name);
            auto bi = std::make_shared<BlockItem>();
            bi->item = decl;
            body.push_back(bi);
            continue;
        }

        // expression statement
        auto expr = parseExpr();
        // consume trailing ; if present
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        auto es = std::make_shared<ExprStmt>();
        es->expr = expr;
        es->span = expr ? expr->span : SourceSpan{};
        auto bi = std::make_shared<BlockItem>();
        bi->item = std::static_pointer_cast<Stmt>(es);
        body.push_back(bi);
    }
    return body;
}

StmtPtr Parser::parseStmt() {
    // not used heavily in current minimal parser
    return nullptr;
}

ExprPtr Parser::parseExpr() {
    auto t = lex.peek();
    if (!t) return nullptr;
    if (t->kind() == TokenKind::IntegerConstant) {
        auto tok = *lex.next();
        auto il = std::make_shared<IntegerLiteral>();
        il->span = tok.span;
        // try to capture raw lexeme if available
        il->raw = tok.lexeme();
        try { il->value = std::stoll(il->raw); } catch (...) { il->value = 0; }
        il->kind = Expr::Kind::Integer;
        return il;
    }
    if (t->kind() == TokenKind::Identifier) {
        auto tok = *lex.next();
        auto id = std::make_shared<IdentifierExpr>();
        id->span = tok.span;
        id->name = tok.lexeme();
        id->kind = Expr::Kind::Ident;
        return id;
    }
    if (t->kind() == TokenKind::StringLiteral) {
        auto tok = *lex.next();
        auto sl = std::make_shared<StringLiteral>();
        sl->span = tok.span;
        sl->value = tok.lexeme();
        sl->kind = Expr::Kind::String;
        return sl;
    }
    // fallback: consume token and make identifier-like node
    auto tok = *lex.next();
    auto id = std::make_shared<IdentifierExpr>();
    id->span = tok.span;
    id->name = tok.lexeme();
    id->kind = Expr::Kind::Ident;
    return id;
}

} // namespace wvmcc::parser
