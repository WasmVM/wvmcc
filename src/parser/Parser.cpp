#include "Parser.hpp"
#include <cassert>
#include <algorithm>
#include <iostream>
#include "ConstExprEval.hpp"

namespace wvmcc::parser {

Parser::Parser(Lexer &lexer) : lex(lexer) {}

static bool initializerIsConstant(const InitializerPtr &init) {
    if (!init) return false;
    if (init->kind == Initializer::Kind::Expr) {
        if (!init->expr) return false;
        if (init->expr->kind == Expr::Kind::String) return true;
        return ConstExprEvaluator::isIntegerConstantExpr(init->expr);
    }
    // list: all clauses' inits must be constant
    for (const auto &cl : init->clauses) {
        if (cl.init) {
            if (!initializerIsConstant(cl.init)) return false;
        } else {
            return false;
        }
        // if designator index present, ensure it's integer-constant
        for (const auto &d : cl.designators) {
            if (d.kind == Designator::Kind::Index) {
                if (!d.index || !ConstExprEvaluator::isIntegerConstantExpr(*d.index)) return false;
            }
        }
    }
    return true;
}

// Evaluate an integer constant expression (very small evaluator).
// Returns std::nullopt if not a constant integer.
// integer constant evaluation is provided by ConstExprEvaluator

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

    auto map_simple = [](const std::string &s) -> std::optional<DeclarationSpecifiers::SimpleTypeSpecifier> {
        using S = DeclarationSpecifiers::SimpleTypeSpecifier;
        if (s == "void") return S::Void;
        if (s == "char") return S::Char;
        if (s == "short") return S::Short;
        if (s == "int") return S::Int;
        if (s == "long") return S::Long;
        if (s == "float") return S::Float;
        if (s == "double") return S::Double;
        if (s == "signed") return S::Signed;
        if (s == "unsigned") return S::Unsigned;
        if (s == "_Bool") return S::Bool;
        if (s == "_Complex") return S::Complex;
        if (s == "_Imaginary") return S::Imaginary;
        return std::nullopt;
    };

    while (auto t = lex.peek()) {
        // If identifier and it's a known typedef-name, treat as type-specifier.
        if (t->kind() == TokenKind::Identifier) {
            if (typedef_names.count(t->lexeme())) {
                DeclarationSpecifiers::TypeSpecifier ts;
                ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::TypedefName;
                ts.text = t->lexeme();
                specs.typeSpecifiers.push_back(ts);
                lex.next();
                continue;
            }
            // otherwise it's the start of a declarator/name
            break;
        }
        if (t->kind() != TokenKind::Keyword) break;
        const std::string s = t->lexeme();
        // storage-class specifiers
        if (storage.count(s)) {
            if (s == "typedef") specs.addStorage(StorageClass::Typedef);
            else if (s == "extern") specs.addStorage(StorageClass::Extern);
            else if (s == "static") specs.addStorage(StorageClass::Static);
            else if (s == "auto") specs.addStorage(StorageClass::Auto);
            else if (s == "register") specs.addStorage(StorageClass::Register);
            lex.next();
            continue;
        }
        // type qualifiers
        if (typequal.count(s)) {
            if (s == "const") specs.addTypeQual(TypeQualifier::Const);
            else if (s == "volatile") specs.addTypeQual(TypeQualifier::Volatile);
            else if (s == "restrict") specs.addTypeQual(TypeQualifier::Restrict);
            else if (s == "_Atomic") specs.addTypeQual(TypeQualifier::Atomic);
            lex.next();
            continue;
        }

        if (types.count(s)) {
                // Handle compound type-specifiers. Special-case struct/union/enum
                // to consume their optional name and optional braced definition.
                if (s == "struct" || s == "union" || s == "enum") {
                    if (s == "enum") {
                        auto ts = parseEnumSpecifier();
                        specs.typeSpecifiers.push_back(ts);
                        continue;
                    } else {
                        auto ts = parseStructOrUnionSpecifier();
                        specs.typeSpecifiers.push_back(ts);
                        continue;
                    }
                }

            // Collect contiguous type-specifier keywords (e.g., "unsigned long int",
            // "long double", "float _Complex") and attempt to store as simple tokens.
            std::vector<std::string> parts;
            while (auto u = lex.peek()) {
                if (u->kind() != TokenKind::Keyword) break;
                const std::string us = u->lexeme();
                if (!types.count(us)) break;
                parts.push_back(us);
                lex.next();
            }
            if (!parts.empty()) {
                bool all_simple = true;
                std::vector<DeclarationSpecifiers::SimpleTypeSpecifier> toks;
                for (auto &p : parts) {
                    auto st = map_simple(p);
                    if (st.has_value()) toks.push_back(*st);
                    else { all_simple = false; break; }
                }
                if (all_simple) {
                    DeclarationSpecifiers::TypeSpecifier nts;
                    nts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
                    nts.simple = toks;
                    specs.typeSpecifiers.push_back(std::move(nts));
                } else {
                    std::string combined = parts[0];
                    for (size_t i = 1; i < parts.size(); ++i) { combined += " "; combined += parts[i]; }
                    DeclarationSpecifiers::TypeSpecifier nts;
                    nts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Other;
                    nts.text = combined;
                    specs.typeSpecifiers.push_back(std::move(nts));
                }
            }
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
            // Collect contiguous type-specifier keywords and store simple tokens
            // (e.g., unsigned,long,int -> [Unsigned,Long,Int]). If the block
            // contains non-simple specifiers (e.g., struct/union/enum) fall back
            // to storing the combined string in `typeSpec`.
            std::vector<std::string> parts;
            while (auto u = lex.peek()) {
                if (u->kind() != TokenKind::Keyword && u->kind() != TokenKind::Identifier) break;
                const std::string us = u->lexeme();
                if (!types.count(us) && !(u->kind() == TokenKind::Identifier && typedef_names.count(us))) break;
                parts.push_back(us);
                lex.next();
            }
            if (!parts.empty()) {
                bool all_simple = true;
                std::vector<DeclarationSpecifiers::SimpleTypeSpecifier> toks;
                for (auto &p : parts) {
                    auto st = map_simple(p);
                    if (st.has_value()) toks.push_back(*st);
                    else {
                        all_simple = false;
                        break;
                    }
                }
                if (all_simple) {
                    DeclarationSpecifiers::TypeSpecifier nts;
                    nts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
                    nts.simple = toks;
                    specs.typeSpecifiers.push_back(std::move(nts));
                } else {
                    // fallback: keep combined textual form (struct/enum or typedef-name)
                    std::string combined = parts[0];
                    for (size_t i = 1; i < parts.size(); ++i) { combined += " "; combined += parts[i]; }
                    DeclarationSpecifiers::TypeSpecifier nts;
                    nts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Other;
                    nts.text = combined;
                    specs.typeSpecifiers.push_back(std::move(nts));
                }
            }
            continue;
        }
        // not a declaration-specifier keyword
        break;
    }
    return specs;
}

DeclarationSpecifiers::TypeSpecifier Parser::parseStructOrUnionSpecifier() {
    DeclarationSpecifiers::TypeSpecifier ts;
    ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion;
    auto su = std::make_shared<StructOrUnionSpecifier>();

    // current token should be 'struct'/'union'/'enum'
    auto kw = lex.peek();
    if (!kw) return ts;
    const std::string k = kw->lexeme();
    su->kind = (k == "struct") ? StructOrUnionSpecifier::Kind::Struct : StructOrUnionSpecifier::Kind::Union;
    // consume keyword
    lex.next();

    // optional identifier
    std::optional<std::string> tagName;
    if (lex.peek() && lex.peek()->kind() == TokenKind::Identifier) {
        tagName = lex.peek()->lexeme();
        su->name = *tagName;
        lex.next();
    }

    // optional body
    bool hasBodyNow = false;
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
        // consume '{'
        lex.next();
        hasBodyNow = true;
        su->hasBody = true;

        // parse members using helper which implements full struct-declaration-list
        su->members = parseStructDeclarationList();
        // consume the trailing '}' if not already consumed by helper (helper stops at '}' and consumes it)
        // parseStructDeclarationList consumes the closing '}' itself, so nothing to do here
    }

    ts.su = su;
    // register or merge into tag registry if we have a tag name
    if (tagName.has_value()) {
        auto it = tag_registry.find(*tagName);
        if (it == tag_registry.end()) {
            // no prior tag: insert current specifier (may be incomplete if no body)
            tag_registry[*tagName] = su;
        } else {
            auto existing = it->second;
            if (existing && existing->hasBody && hasBodyNow) {
                // duplicate tag definition
                wvmcc::Diagnostic d;
                d.severity = wvmcc::Diagnostic::Severity::Error;
                d.message = "redefinition of struct/union tag '" + *tagName + "'";
                d.span = kw->span;
                diagnostics.push_back(std::move(d));
            } else if (existing && !existing->hasBody && hasBodyNow) {
                // complete previously incomplete tag: copy members into the registered specifier
                existing->hasBody = true;
                existing->members = su->members;
                // point ts.su to the canonical registered specifier
                ts.su = existing;
            } else {
                // keep registered specifier (either both incomplete or existing complete and new incomplete)
                ts.su = existing;
            }
        }
    }
    return ts;
}

DeclarationSpecifiers::TypeSpecifier Parser::parseEnumSpecifier() {
    DeclarationSpecifiers::TypeSpecifier ts;
    ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Enum;
    auto en = std::make_shared<DeclarationSpecifiers::TypeSpecifier::EnumSpecifier>();

    // current token should be 'enum'
    auto kw = lex.peek();
    if (!kw) return ts;
    // consume 'enum'
    lex.next();

    // optional identifier
    std::optional<std::string> tagName;
    if (lex.peek() && lex.peek()->kind() == TokenKind::Identifier) {
        tagName = lex.peek()->lexeme();
        en->name = *tagName;
        lex.next();
    }

    // optional body
    bool hasBodyNow = false;
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
        // consume '{'
        lex.next();
        hasBodyNow = true;
        en->hasBody = true;

        // parse enumerator list
        while (auto p = lex.peek()) {
            if (p->kind() == TokenKind::Punctuator && p->lexeme() == "}") { lex.next(); break; }

            // expect identifier
            if (p->kind() == TokenKind::Identifier) {
                DeclarationSpecifiers::TypeSpecifier::EnumSpecifier::Enumerator ev;
                ev.name = p->lexeme();
                lex.next();
                // optional '=' constant-expression
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "=") {
                    lex.next();
                    ev.value = parseExpr();
                }
                en->enumerators.push_back(std::move(ev));
                // optional trailing comma
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
                    lex.next();
                    // allow trailing comma before '}'
                    continue;
                }
                continue;
            }
            // if something else, attempt to recover by consuming token
            lex.next();
        }
    }

    ts.en = en;
    // register or merge into enum tag registry if we have a tag name
    if (tagName.has_value()) {
        auto it = enum_tag_registry.find(*tagName);
        if (it == enum_tag_registry.end()) {
            enum_tag_registry[*tagName] = en;
        } else {
            auto existing = it->second;
            if (existing && existing->hasBody && hasBodyNow) {
                wvmcc::Diagnostic d;
                d.severity = wvmcc::Diagnostic::Severity::Error;
                d.message = "redefinition of enum tag '" + *tagName + "'";
                d.span = kw->span;
                diagnostics.push_back(std::move(d));
            } else if (existing && !existing->hasBody && hasBodyNow) {
                existing->hasBody = true;
                existing->enumerators = en->enumerators;
                ts.en = existing;
            } else {
                ts.en = existing;
            }
        }
    }

    return ts;
}

StructDeclarator Parser::parseStructDeclarator() {
    StructDeclarator sd;
    // optional declarator
    if (lex.peek() && (lex.peek()->kind() == TokenKind::Identifier || lex.peek()->kind() == TokenKind::Punctuator)) {
        // accept identifier as declarator-id
        if (lex.peek()->kind() == TokenKind::Identifier) {
            auto id = make_ast<Declarator>();
            id->id.name = lex.peek()->lexeme();
            sd.declarator = id;
            lex.next();
        } else {
            // other declarator forms (not fully implemented): leave declarator null and continue
        }
    }

    // optional bit-field width
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":") {
        lex.next();
        sd.bitfieldWidth = parseExpr();
    }
    return sd;
}

DeclaratorPtr Parser::parseDeclarator() {
    DeclaratorPtr d = nullptr;

    // parse pointer sequence: '*' type-qualifier-listopt
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "*") {
        // consume '*'
        lex.next();
        TypeQualifier q = TypeQualifier::None;
        // optional type qualifiers
        while (lex.peek() && lex.peek()->kind() == TokenKind::Keyword) {
            const std::string k = lex.peek()->lexeme();
            if (k == "const") { q |= TypeQualifier::Const; lex.next(); continue; }
            if (k == "volatile") { q |= TypeQualifier::Volatile; lex.next(); continue; }
            if (k == "restrict") { q |= TypeQualifier::Restrict; lex.next(); continue; }
            if (k == "_Atomic") { q |= TypeQualifier::Atomic; lex.next(); continue; }
            break;
        }
        auto p = make_ast<Declarator>();
        p->kind = Declarator::Kind::Pointer;
        p->ptrQual = q;
        p->inner = d;
        d = p;
    }

    // direct-declarator: identifier or ( declarator )
    if (lex.peek() && lex.peek()->kind() == TokenKind::Identifier) {
        auto id = make_ast<Declarator>();
        id->kind = Declarator::Kind::Identifier;
        id->id.name = lex.peek()->lexeme();
        id->inner = d;
        lex.next();
        d = id;
    } else if (acceptPunct("(")) {
        auto inner = parseDeclarator();
        acceptPunct(")");
        if (inner) {
            inner->inner = d;
            d = inner;
        } else {
            auto nd = make_ast<Declarator>();
            nd->kind = Declarator::Kind::Nested;
            nd->inner = d;
            d = nd;
        }
    }

    // handle trailing suffixes: arrays and function parameter lists
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
        auto p = lex.peek();
        if (p->lexeme() == "[") {
            lex.next();
            bool isStatic = false;
            TypeQualifier qual = TypeQualifier::None;
            if (lex.peek() && lex.peek()->kind() == TokenKind::Keyword && lex.peek()->lexeme() == "static") { isStatic = true; lex.next(); }
            // optional qualifiers
            while (lex.peek() && lex.peek()->kind() == TokenKind::Keyword) {
                const std::string k = lex.peek()->lexeme();
                if (k == "const") { qual |= TypeQualifier::Const; lex.next(); continue; }
                if (k == "volatile") { qual |= TypeQualifier::Volatile; lex.next(); continue; }
                if (k == "restrict") { qual |= TypeQualifier::Restrict; lex.next(); continue; }
                if (k == "_Atomic") { qual |= TypeQualifier::Atomic; lex.next(); continue; }
                break;
            }

            // [ * ] form
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "*") {
                lex.next();
                acceptPunct("]");
                auto arr = make_ast<Declarator>();
                arr->kind = Declarator::Kind::Array;
                arr->array.isStar = true;
                arr->array.isStatic = isStatic;
                arr->array.qual = qual;
                arr->inner = d;
                d = arr;
                continue;
            }

            std::optional<ExprPtr> size;
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "]")) {
                size = parseExpr();
            }
            acceptPunct("]");
            auto arr = make_ast<Declarator>();
            arr->kind = Declarator::Kind::Array;
            arr->array.size = size;
            arr->array.isStatic = isStatic;
            arr->array.qual = qual;
            arr->inner = d;
            d = arr;
            continue;
        } else if (p->lexeme() == "(") {
            lex.next();
            std::vector<Parameter> params;
            std::vector<std::string> idlist;
            bool hasParamTypeList = false;

            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                // heuristics: if it starts with a type keyword or a typedef-name treat as parameter-type-list
                if (lex.peek() && (lex.peek()->kind() == TokenKind::Keyword || (lex.peek()->kind() == TokenKind::Identifier && typedef_names.count(lex.peek()->lexeme())))) {
                    hasParamTypeList = true;
                    while (true) {
                        auto pspecs = parseDeclarationSpecifiers();
                        Parameter param;
                        if (lex.peek() && (lex.peek()->kind() == TokenKind::Identifier || (lex.peek()->kind() == TokenKind::Punctuator && (lex.peek()->lexeme() == "(" || lex.peek()->lexeme() == "*")) )) {
                            param.declarator = parseDeclarator();
                        } else {
                            param.declarator = nullptr; // abstract-declarator not implemented
                        }
                        params.push_back(param);
                        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
                            lex.next();
                            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") break;
                            continue;
                        }
                        break;
                    }
                } else {
                    // identifier-list (K&R style)
                    while (true) {
                        if (lex.peek() && lex.peek()->kind() == TokenKind::Identifier) {
                            idlist.push_back(lex.peek()->lexeme());
                            lex.next();
                        } else {
                            break;
                        }
                        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
                            lex.next();
                            continue;
                        }
                        break;
                    }
                }
            }

            acceptPunct(")");
            auto fn = make_ast<Declarator>();
            fn->kind = Declarator::Kind::Function;
            fn->function.params = params;
            fn->function.hasParamTypeList = hasParamTypeList;
            fn->function.identifierList = idlist;
            fn->inner = d;
            d = fn;
            continue;
        } else break;
    }

    return d;
}

std::vector<StructMember> Parser::parseStructDeclarationList() {
    std::vector<StructMember> members;
    // expect that '{' has already been consumed by caller
    while (auto p = lex.peek()) {
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == "}") { lex.next(); break; }

        // parse specifiers
        auto memberSpecs = parseDeclarationSpecifiers();

        // if next is ';' -> anonymous member with no declarators
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ";") {
            lex.next();
            StructMember m;
            m.specifiers = memberSpecs;
            members.push_back(std::move(m));
            continue;
        }

        // otherwise parse struct-declarator-list
        StructMember m;
        m.specifiers = memberSpecs;
        while (true) {
            StructDeclarator sd = parseStructDeclarator();
            m.declarators.push_back(std::move(sd));

            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
                lex.next();
                continue;
            }
            break;
        }

        // require terminating ';'
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ";") lex.next();

        members.push_back(std::move(m));
    }
    return members;
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

    // Handle _Static_assert (C 6.7.10): _Static_assert ( constant-expression , string-literal ) ;
    if (lex.peek() && lex.peek()->kind() == TokenKind::Keyword && lex.peek()->lexeme() == "_Static_assert") {
        // consume keyword
        lex.next();
        // expect '('
        if (!acceptPunct("(")) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "expected '(' after _Static_assert";
            if (lex.peek()) d.span = lex.peek()->span;
            diagnostics.push_back(std::move(d));
            // recover: skip to next ';'
            while (lex.peek() && !(lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";")) lex.next();
            if (lex.peek()) lex.next();
            return nullptr;
        }

        // parse constant-expression
        auto expr = parseExpr();
        // expect comma
        if (!(lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==",")) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "expected ',' in _Static_assert";
            if (lex.peek()) d.span = lex.peek()->span;
            diagnostics.push_back(std::move(d));
        } else {
            lex.next();
        }

        // expect string-literal
        std::string msg;
        if (lex.peek() && lex.peek()->kind() == TokenKind::StringLiteral) {
            msg = lex.peek()->lexeme();
            lex.next();
        } else {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "expected string literal in _Static_assert";
            if (lex.peek()) d.span = lex.peek()->span;
            diagnostics.push_back(std::move(d));
        }

        // expect ')'
        if (!(lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==")")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after _Static_assert"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else {
            lex.next();
        }

        // expect ';'
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();

        // Evaluate constant-expression
        auto val = ConstExprEvaluator::evalIntegerConstantExpr(expr);
        if (!val.has_value()) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "_Static_assert requires an integer constant expression";
            if (expr) d.span = expr->span;
            diagnostics.push_back(std::move(d));
        } else if (*val == 0) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = std::string("static assertion failed: ") + msg;
            if (expr) d.span = expr->span;
            diagnostics.push_back(std::move(d));
        }

        return nullptr; // static assert is its own external declaration but we don't create AST node for it
    }

    // Try to parse an optional declarator (may be null for declarations like "struct S;")
    DeclaratorPtr decl = nullptr;
    // Only attempt to parse a declarator if next token could start one
    if (lex.peek()) {
        if (lex.peek()->kind() == TokenKind::Identifier || (lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") || (lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "*")) {
            decl = parseDeclarator();
        }
    }

    // Early constraint check: storage-class specifiers 'auto' and 'register' are invalid
    // in external declarations (C standard 6.9).
    if (specs.hasStorage(StorageClass::Auto) || specs.hasStorage(StorageClass::Register)) {
        if (specs.hasStorage(StorageClass::Auto)) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "storage-class specifier 'auto' is not allowed in external declarations";
            if (decl) d.span = decl->span; 
            diagnostics.push_back(std::move(d));
        }
        if (specs.hasStorage(StorageClass::Register)) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "storage-class specifier 'register' is not allowed in external declarations";
            if (decl) d.span = decl->span;
            diagnostics.push_back(std::move(d));
        }
    }

    // If we parsed a declarator and it (or its nested form) is a function declarator,
    // determine whether this is a function definition (followed by '{') or a prototype/declaration.
    std::string name;
    if (decl) {
        // find the identifier in nested declarator chain
        auto cur = decl;
        while (cur) {
            if (cur->kind == Declarator::Kind::Identifier && !cur->id.name.empty()) { name = cur->id.name; break; }
            cur = cur->inner.has_value() ? *cur->inner : nullptr;
        }
    }

    // Helper: find if declarator chain contains a Function kind
    auto containsFunction = [&](const DeclaratorPtr &dptr)->bool {
        auto cur = dptr;
        while (cur) {
            if (cur->kind == Declarator::Kind::Function) return true;
            cur = cur->inner.has_value() ? *cur->inner : nullptr;
        }
        return false;
    };

    if (decl && containsFunction(decl)) {
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
            auto f = parseFunctionDef(specs, decl);
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
                        it->second = std::make_pair(f->span, true);
                    }
                } else {
                    internal_definitions[name] = std::make_pair(f->span, true);
                }
            }

            return ext;
        } else {
                auto d = parseDeclaration(specs, decl);
                if (!d) return nullptr;
                // static/thread storage duration initializers must be constant (C 6.7.9 constraint 4)
                if (d->initializer.has_value() && (specs.hasStorage(StorageClass::Static) /*|| specs.hasStorage(StorageClass::ThreadLocal)*/)) {
                    if (!initializerIsConstant(*d->initializer)) {
                        wvmcc::Diagnostic diag;
                        diag.severity = wvmcc::Diagnostic::Severity::Error;
                        diag.message = "initializer for object with static storage duration must be constant expression or string literal";
                        diag.span = d->span;
                        diagnostics.push_back(std::move(diag));
                    }
                }
                auto ext = make_ast_with_span<ExternalDecl>(d->span);
                ext->decl = d;
                return ext;
        }
    }

    // if there was no declarator and the next token is a semicolon, treat as declaration with no declarator
    if (!decl && lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ";") {
        auto d = parseDeclaration(specs, "");
        if (!d) return nullptr;
        auto ext = make_ast_with_span<ExternalDecl>(d->span);
        ext->decl = d;
        return ext;
    }

    // otherwise, if we have a declarator (non-function) treat as declaration
    if (decl) {
        auto d = parseDeclaration(specs, decl);
        if (!d) return nullptr;
        // If this declaration has static/thread storage duration, its initializer
        // expressions must be constant expressions or string literals (C 6.7.9 constraint 4).
        if (d->initializer.has_value() && (specs.hasStorage(StorageClass::Static) /*|| specs.hasStorage(StorageClass::ThreadLocal)*/)) {
            if (!initializerIsConstant(*d->initializer)) {
                wvmcc::Diagnostic diag;
                diag.severity = wvmcc::Diagnostic::Severity::Error;
                diag.message = "initializer for object with static storage duration must be constant expression or string literal";
                diag.span = d->span;
                diagnostics.push_back(std::move(diag));
            }
        }
        // For object declarations with internal linkage (static): tentative/definitive semantics
        if (!decl->id.name.empty() && specs.hasStorage(StorageClass::Static)) {
            std::string nm = decl->id.name;
            bool is_definitive = d->initializer.has_value();
            auto it = internal_definitions.find(nm);
            if (is_definitive) {
                if (it != internal_definitions.end() && it->second.second) {
                    wvmcc::Diagnostic diag;
                    diag.severity = wvmcc::Diagnostic::Severity::Error;
                    diag.message = "duplicate internal definition of '" + nm + "' in translation unit";
                    diag.span = d->span;
                    diagnostics.push_back(std::move(diag));
                } else if (it != internal_definitions.end()) {
                    it->second = std::make_pair(d->span, true);
                } else {
                    internal_definitions[nm] = std::make_pair(d->span, true);
                }
            } else {
                if (it == internal_definitions.end()) internal_definitions[nm] = std::make_pair(d->span, false);
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

FunctionDefPtr Parser::parseFunctionDef(const DeclarationSpecifiers& specs, const DeclaratorPtr &decl) {
    if (!acceptPunct("{")) return nullptr;
    auto f = make_ast<FunctionDef>();
    f->specifiers = specs;
    f->declarator = decl;
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
            // initializer: could be an assignment-expression or a braced initializer-list
            lex.next();
            decl->initializer = parseInitializer();
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

DeclarationPtr Parser::parseDeclaration(const DeclarationSpecifiers& specs, const DeclaratorPtr &declr) {
    auto decl = make_ast<Declaration>();
    decl->specifiers = specs;
    if (declr) decl->declarator = declr;

    while (auto p = lex.peek()) {
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == ";") { lex.next(); break; }
        if (p->kind() == TokenKind::Punctuator && p->lexeme() == "=") {
            // initializer: could be an assignment-expression or a braced initializer-list
            lex.next();
            decl->initializer = parseInitializer();
            // consume until semicolon for simple recovery (initializer parser consumes balanced braces)
            while (auto q = lex.peek()) { if (q->kind()==TokenKind::Punctuator && q->lexeme()==";") { lex.next(); break; } lex.next(); }
            break;
        }
        lex.next();
    }
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
            // C 6.7.9 constraint 5: if declaration has block scope and the identifier has
            // external or internal linkage, the declaration shall have no initializer.
            if (decl && decl->initializer.has_value() && (specs.hasStorage(StorageClass::Extern) || specs.hasStorage(StorageClass::Static))) {
                wvmcc::Diagnostic diag;
                diag.severity = wvmcc::Diagnostic::Severity::Error;
                diag.message = "declaration at block scope with external/internal linkage shall not have an initializer";
                diag.span = decl->span;
                diagnostics.push_back(std::move(diag));
            }
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

InitializerPtr Parser::parseInitializer() {
    // If next token is a '{', parse an initializer-list
    auto init = make_ast<Initializer>();
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
        init->kind = Initializer::Kind::List;
        // consume '{'
        lex.next();
        while (true) {
            // handle optional trailing comma before '}'
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "}") {
                lex.next();
                init->trailingComma = false;
                break;
            }

            // parse a clause: optional designators followed by '=' (designationopt)
            InitClause clause;
            // collect designators
            while (lex.peek() && ((lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="[") || (lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="."))) {
                clause.designators.push_back(parseDesignator());
            }
            // if designators present, expect '='
            if (!clause.designators.empty()) {
                if (!(lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="=")) {
                    wvmcc::Diagnostic d;
                    d.severity = wvmcc::Diagnostic::Severity::Error;
                    d.message = "expected '=' after designator in initializer";
                    if (lex.peek()) d.span = lex.peek()->span;
                    diagnostics.push_back(std::move(d));
                } else {
                    lex.next();
                }
            }

            // parse the initializer (could be nested list or expression)
            clause.init = parseInitializer();
            init->clauses.push_back(clause);

            // if next is ',', consume and continue; if next is '}', break
            if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==",") {
                lex.next();
                // if next is '}', it's a trailing comma
                if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="}") {
                    init->trailingComma = true;
                    lex.next();
                    break;
                }
                continue;
            }
            // no comma: expect closing '}'
            if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="}") {
                lex.next();
                break;
            }
            // unexpected token: try to recover
            if (!lex.peek()) break;
            lex.next();
        }
        return init;
    }

    // Otherwise parse as an assignment-expression
    init->kind = Initializer::Kind::Expr;
    init->expr = parseExpr();
    return init;
}

Designator Parser::parseDesignator() {
    Designator d;
    if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="[") {
        // array-index designator
        lex.next();
        auto e = parseExpr();
        d.kind = Designator::Kind::Index;
        d.index = e;
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="]") lex.next();
            // designator index must be an integer constant expression (C 6.7.9 constraint 6)
            if (e && !ConstExprEvaluator::isIntegerConstantExpr(e)) {
                wvmcc::Diagnostic diag;
                diag.severity = wvmcc::Diagnostic::Severity::Error;
                diag.message = "designator index must be an integer constant expression";
                diag.span = e->span;
                diagnostics.push_back(std::move(diag));
            }
            return d;
    }
    if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==".") {
        // member designator
        lex.next();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Identifier) {
            d.kind = Designator::Kind::Member;
            d.member = lex.peek()->lexeme();
            lex.next();
        } else {
            wvmcc::Diagnostic diag;
            diag.severity = wvmcc::Diagnostic::Severity::Error;
            diag.message = "expected identifier after '.' in designator";
            if (lex.peek()) diag.span = lex.peek()->span;
            diagnostics.push_back(std::move(diag));
        }
        return d;
    }
    return d;
}

} // namespace wvmcc::parser
