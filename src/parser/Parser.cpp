#include "Parser.hpp"
#include <cassert>
#include <algorithm>

namespace wvmcc::parser {

Parser::Parser(Lexer &lexer) : lex(lexer) {}

DeclarationSpecifiers Parser::parseDeclarationSpecifiers() {
    DeclarationSpecifiers specs;

    static const std::unordered_set<std::string> storage = {"typedef","extern","static","auto","register"};
    static const std::unordered_set<std::string> typequal = {"const","volatile","restrict","_Atomic"};
    static const std::unordered_set<std::string> funcspec = {"inline","_Noreturn"};
    static const std::unordered_set<std::string> alignspec = {"_Alignas"};
    static const std::unordered_set<std::string> types = {
        "void","char","short","int","long","float","double","signed","unsigned",
        "_Bool","_Complex","_Imaginary","struct","union","enum"
    };

    while (auto t = lex.peek()) {
        // If identifier and it's a known typedef-name, treat as type-specifier.
        if (t->kind() == TokenKind::Identifier) {
            if (typedef_names.count(t->lexeme())) {
                specs.typeSpec.push_back(t->lexeme());
                lex.next();
                continue;
            }
            // otherwise it's the start of a declarator/name
            break;
        }
        if (t->kind() != TokenKind::Keyword) break;
        const std::string s = t->lexeme();
        if (storage.count(s)) {
            if (s == "typedef") specs.addStorage(StorageClass::Typedef);
            else if (s == "extern") specs.addStorage(StorageClass::Extern);
            else if (s == "static") specs.addStorage(StorageClass::Static);
            else if (s == "auto") specs.addStorage(StorageClass::Auto);
            else if (s == "register") specs.addStorage(StorageClass::Register);
            else if (s == "_Thread_local") specs.addStorage(StorageClass::ThreadLocal);
            lex.next();
            continue;
        }
        if (typequal.count(s)) {
            if (s == "const") specs.addTypeQual(TypeQualifier::Const);
            else if (s == "volatile") specs.addTypeQual(TypeQualifier::Volatile);
            else if (s == "restrict") specs.addTypeQual(TypeQualifier::Restrict);
            else if (s == "_Atomic") specs.addTypeQual(TypeQualifier::Atomic);
            lex.next();
            continue;
        }
        if (funcspec.count(s)) {
            if (s == "inline") specs.addFuncSpec(FunctionSpecifier::Inline);
            else if (s == "_Noreturn") specs.addFuncSpec(FunctionSpecifier::NoReturn);
            lex.next();
            continue;
        }
        if (alignspec.count(s)) {
            specs.alignSpec.push_back(s);
            lex.next();
            continue;
        }
        if (types.count(s)) {
            specs.typeSpec.push_back(s);
            lex.next();
            continue;
        }
        // not a declaration-specifier keyword
        break;
    }
    return specs;
}

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
    auto tu = make_ast<TranslationUnit>();
    while (lex.peek() != std::nullopt) {
        // parse external declaration; on error we recover and continue
        auto ext = parseExternalDecl();
        if (ext) tu->externals.push_back(ext);
    }
    return tu;
}

ExternalDeclPtr Parser::parseExternalDecl() {
    // gather specifiers (keywords like 'int', 'static', etc.)
    auto specs = parseDeclarationSpecifiers();

    auto nameTok = lex.peek();
    if (!nameTok) return nullptr;

    std::string name;
    if (nameTok->kind() == TokenKind::Identifier) {
        name = nameTok->lexeme();
        lex.next();
    }

    // Early constraint check: storage-class specifiers 'auto' and 'register' are invalid
    // in external declarations (C standard 6.9).
    if (specs.hasStorage(StorageClass::Auto) || specs.hasStorage(StorageClass::Register)) {
        if (specs.hasStorage(StorageClass::Auto)) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "storage-class specifier 'auto' is not allowed in external declarations";
            if (nameTok) d.span = nameTok->span;
            diagnostics.push_back(std::move(d));
        }
        if (specs.hasStorage(StorageClass::Register)) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "storage-class specifier 'register' is not allowed in external declarations";
            if (nameTok) d.span = nameTok->span;
            diagnostics.push_back(std::move(d));
        }
    }

    // if followed by '(' => function (params ignored for now)
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") {
        // consume until matching ')'
        lex.next();
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

        // if next is '{' parse body -> function definition
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
            auto f = parseFunctionDef(specs, name);
            if (!f) return nullptr;
            auto ext = make_ast_with_span<ExternalDecl>(f->span);
            ext->decl = f;

            // duplicate internal definition check for 'static' functions
            if (!name.empty() && specs.hasStorage(StorageClass::Static)) {
                auto it = internal_definitions.find(name);
                bool is_definitive = true; // function definition is definitive
                if (it != internal_definitions.end()) {
                    if (it->second.second && is_definitive) {
                        wvmcc::Diagnostic d;
                        d.severity = wvmcc::Diagnostic::Severity::Error;
                        d.message = "duplicate internal definition of '" + name + "' in translation unit";
                        d.span = f->span;
                        diagnostics.push_back(std::move(d));
                    } else {
                        // upgrade tentative -> definitive
                        it->second = std::make_pair(f->span, true);
                    }
                } else {
                    internal_definitions[name] = std::make_pair(f->span, true);
                }
            }

            return ext;
        } else {
            // prototype declaration
            auto d = parseDeclaration(specs, name);
            if (!d) return nullptr;
            auto ext = make_ast_with_span<ExternalDecl>(d->span);
            ext->decl = d;
            return ext;
        }
    }

    // otherwise if the name exists, treat as declaration
    if (!name.empty()) {
        auto d = parseDeclaration(specs, name);
        if (!d) return nullptr;

        // For object declarations with internal linkage (static): consider tentative/definitive semantics
        if (specs.hasStorage(StorageClass::Static)) {
            bool is_definitive = d->initializer.has_value();
            auto it = internal_definitions.find(name);
            if (is_definitive) {
                if (it != internal_definitions.end() && it->second.second) {
                    // previous definitive exists -> error
                    wvmcc::Diagnostic diag;
                    diag.severity = wvmcc::Diagnostic::Severity::Error;
                    diag.message = "duplicate internal definition of '" + name + "' in translation unit";
                    diag.span = d->span;
                    diagnostics.push_back(std::move(diag));
                } else if (it != internal_definitions.end()) {
                    // upgrade tentative -> definitive
                    it->second = std::make_pair(d->span, true);
                } else {
                    internal_definitions[name] = std::make_pair(d->span, true);
                }
            } else {
                // tentative definition: record if no definitive recorded yet
                if (it == internal_definitions.end()) internal_definitions[name] = std::make_pair(d->span, false);
            }
        }

        auto ext = make_ast_with_span<ExternalDecl>(d->span);
        ext->decl = d;
        return ext;
    }

    // fallback recovery: emit a diagnostic and synchronize to the next ';'
    auto t = lex.next();
    if (!t) return nullptr;
    wvmcc::Diagnostic d;
    d.severity = wvmcc::Diagnostic::Severity::Error;
    d.message = "unexpected token '" + t->lexeme() + "' in translation unit, synchronizing";
    d.span = t->span;
    diagnostics.push_back(std::move(d));

    // consume until a semicolon or EOF to recover to a likely next external decl
    while (auto p = lex.peek()) {
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == ";") { lex.next(); break; }
        // stop before a '{' so higher-level logic can still detect function bodies if applicable
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == "{") break;
        lex.next();
    }

    // return nullptr to indicate nothing valid parsed here (caller will continue)
    return nullptr;
}

FunctionDefPtr Parser::parseFunctionDef(const DeclarationSpecifiers& specs, const std::string &name) {
    if (!acceptPunct("{")) return nullptr;
    auto f = make_ast<FunctionDef>();
    f->specifiers = specs;
    auto d = make_ast<Declarator>();
    d->id.name = name;
    f->declarator = d;
    f->body = parseCompoundBody();
    return f;
}

DeclarationPtr Parser::parseDeclaration(const DeclarationSpecifiers& specs, const std::string &name) {
    auto decl = make_ast<Declaration>();
    decl->specifiers = specs;
    if (!name.empty()) {
        auto d = make_ast<Declarator>();
        d->id.name = name;
        decl->declarator = d;
    }

    while (auto p = lex.peek()) {
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == ";") { lex.next(); break; }
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == "=") {
            lex.next();
            decl->initializer = parseExpr();
            while (auto q = lex.peek()) { if (q->kind()==TokenKind::Punctuator && q->lexeme()==";") { lex.next(); break; } lex.next(); }
            break;
        }
        lex.next();
    }
    // If this declaration introduces a typedef-name, record it for future
    // recognition in `parseDeclarationSpecifiers()`.
    if (specs.hasStorage(StorageClass::Typedef)) {
        if (decl->declarator && !decl->declarator->id.name.empty()) {
            typedef_names.insert(decl->declarator->id.name);
        }
    }

    return decl;
}



std::vector<BlockItemPtr> Parser::parseCompoundBody() {
    std::vector<BlockItemPtr> body;
    while (auto p = lex.peek()) {
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == "}") { lex.next(); break; }

        if (p->kind() == TokenKind::Keyword && p->lexeme() == "return") {
            lex.next();
            auto rs = make_ast<ReturnStmt>();
            rs->span = p->span;
            rs->value = parseExpr();
            if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
            auto bi = make_ast<BlockItem>();
            bi->item = std::static_pointer_cast<Stmt>(rs);
            body.push_back(bi);
            continue;
        }

        if (p->kind() == TokenKind::Keyword || p->kind() == TokenKind::Identifier) {
            auto specs = parseDeclarationSpecifiers();
            std::string name;
            if (lex.peek() && lex.peek()->kind()==TokenKind::Identifier) { name = lex.peek()->lexeme(); lex.next(); }
            auto decl = parseDeclaration(specs, name);
            auto bi = make_ast<BlockItem>();
            bi->item = decl;
            body.push_back(bi);
            continue;
        }

        auto expr = parseExpr();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        auto es = make_ast<ExprStmt>();
        es->expr = expr;
        es->span = expr ? expr->span : SourceSpan{};
        auto bi = make_ast<BlockItem>();
        bi->item = std::static_pointer_cast<Stmt>(es);
        body.push_back(bi);
    }
    return body;
}

StmtPtr Parser::parseStmt() {
    return nullptr;
}

ExprPtr Parser::parseExpr() {
    auto t = lex.peek();
    if (!t) return nullptr;
    if (t->kind() == TokenKind::IntegerConstant) {
        auto tok = *lex.next();
        auto il = make_ast<IntegerLiteral>();
        il->span = tok.span;
        il->raw = tok.lexeme();
        try { il->value = std::stoll(il->raw); } catch (...) { il->value = 0; }
        il->kind = Expr::Kind::Integer;
        return il;
    }
    if (t->kind() == TokenKind::Identifier) {
        auto tok = *lex.next();
        auto id = make_ast<IdentifierExpr>();
        id->span = tok.span;
        id->name = tok.lexeme();
        id->kind = Expr::Kind::Ident;
        return id;
    }
    if (t->kind() == TokenKind::StringLiteral) {
        auto tok = *lex.next();
        auto sl = make_ast<StringLiteral>();
        sl->span = tok.span;
        sl->value = tok.lexeme();
        sl->kind = Expr::Kind::String;
        return sl;
    }
    auto tok = *lex.next();
    auto id = make_ast<IdentifierExpr>();
    id->span = tok.span;
    id->name = tok.lexeme();
    id->kind = Expr::Kind::Ident;
    return id;
}

} // namespace wvmcc::parser
