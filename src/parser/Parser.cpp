#include "Parser.hpp"
#include <cassert>
#include <algorithm>
#include <iostream>
#include "ConstExprEval.hpp"

namespace wvmcc::parser {

Parser::Parser(Lexer &lexer) : lex(lexer) {
    // Predefined builtin typedef-names. __builtin_va_list is the variadic
    // arg-list cookie; we represent it as a Wasm i64 (a pointer to the
    // current variadic-arg slot on the shadow stack).
    typedef_names.insert("__builtin_va_list");
}

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

std::vector<GnuAttribute> Parser::parseGnuAttributeSpecifierList() {
    std::vector<GnuAttribute> result;
    while (auto p = lex.peek()) {
        if (p->kind() != TokenKind::Identifier || p->lexeme() != "__attribute__") break;
        lex.next(); // consume __attribute__
        if (!acceptPunct("(") || !acceptPunct("(")) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "expected '((' after __attribute__";
            if (lex.peek()) d.span = lex.peek()->span;
            diagnostics.push_back(std::move(d));
            return result;
        }
        // attribute-list: zero or more attributes separated by ','
        while (auto q = lex.peek()) {
            if (q->kind() == TokenKind::Punctuator && q->lexeme() == ")") break;
            // attribute name: identifier or keyword (GCC permits keywords like 'const')
            if (q->kind() != TokenKind::Identifier && q->kind() != TokenKind::Keyword) {
                // unrecognized token in attribute list; skip to recover
                lex.next();
                continue;
            }
            GnuAttribute attr;
            attr.name = q->lexeme();
            lex.next();
            if (acceptPunct("(")) {
                while (auto r = lex.peek()) {
                    if (r->kind() == TokenKind::Punctuator && r->lexeme() == ")") break;
                    if (r->kind() == TokenKind::StringLiteral) {
                        attr.stringArgs.push_back(r->lexeme());
                    }
                    // string literals, identifiers, integers: consume; commas separate them
                    lex.next();
                    if (auto sep = lex.peek(); sep && sep->kind() == TokenKind::Punctuator && sep->lexeme() == ",") {
                        lex.next();
                    }
                }
                acceptPunct(")");
            }
            result.push_back(std::move(attr));
            // optional comma between attributes
            if (auto sep = lex.peek(); sep && sep->kind() == TokenKind::Punctuator && sep->lexeme() == ",") {
                lex.next();
            }
        }
        // expect '))'
        acceptPunct(")");
        acceptPunct(")");
    }
    return result;
}

DeclarationSpecifiers Parser::parseDeclarationSpecifiers() {
    DeclarationSpecifiers specs;

    static const std::unordered_set<std::string> storage = {"typedef","extern","static","auto","register","_Thread_local"};
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
                // Special-case the builtin va_list typedef: represent as `long`
                // so the rest of the type system handles it as an i64 scalar.
                if (t->lexeme() == "__builtin_va_list") {
                    DeclarationSpecifiers::TypeSpecifier ts;
                    ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
                    ts.simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Long);
                    specs.typeSpecifiers.push_back(ts);
                    lex.next();
                    continue;
                }
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
            else if (s == "_Thread_local") specs.addStorage(StorageClass::ThreadLocal);
            lex.next();
            continue;
        }
        // type qualifiers and special-case `_Atomic(type)` form
        if (typequal.count(s)) {
            if (s == "const") { specs.addTypeQual(TypeQualifier::Const); lex.next(); continue; }
            else if (s == "volatile") { specs.addTypeQual(TypeQualifier::Volatile); lex.next(); continue; }
            else if (s == "restrict") { specs.addTypeQual(TypeQualifier::Restrict); lex.next(); continue; }
            else if (s == "_Atomic") {
                // Peek to see if this is the `_Atomic(type)` form (type-specifier)
                // Consume the `_Atomic` token and inspect next token.
                lex.next();
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") {
                    // consume '('
                    lex.next();
                    // parse an inner declaration-specifiers representing the wrapped type
                    auto innerSpecs = parseDeclarationSpecifiers();
                    // expect ')'
                    if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                        wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after _Atomic(type)"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
                    } else {
                        lex.next();
                    }
                    DeclarationSpecifiers::TypeSpecifier nts;
                    nts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Atomic;
                    // store the parsed inner specs so later passes can inspect inner tags
                    nts.atomicInner = std::make_shared<DeclarationSpecifiers>(std::move(innerSpecs));
                    specs.typeSpecifiers.push_back(std::move(nts));
                    continue;
                } else {
                    // plain qualifier form: `_Atomic` as type-qualifier
                    specs.addTypeQual(TypeQualifier::Atomic);
                    continue;
                }
            }
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
            // parse _Alignas( constant-expression ) and store parsed expr
            lex.next();
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") {
                lex.next();
                // parse an inner constant-expression (or arbitrary expression)
                auto expr = parseConditionalExpression();
                // expect ')'
                if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                    wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after _Alignas expression"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
                } else {
                    lex.next();
                }
                // record both textual placeholder and parsed expr for Semantic to evaluate
                specs.alignSpec.push_back(s); // keep marker for legacy uses
                specs.alignExprs.push_back(expr);
            } else {
                // malformed: treat as simple spec for recovery
                specs.alignSpec.push_back(s);
            }
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
                // duplicate tag definition -- move reporting to Semantic
                // leave registry unchanged
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
                    ev.value = parseConditionalExpression();
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
                // duplicate enum tag definition - defer to Semantic
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
    // optional declarator. A struct-declarator may omit the declarator only
    // for an anonymous bit-field (`: width`); otherwise parse a full
    // declarator so pointer / array / function members are handled (and, in
    // particular, so the lexer always advances — a `*p` member used to fall
    // through the old identifier-only path consuming nothing, hanging the
    // enclosing parseStructDeclarationList loop forever).
    if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":")) {
        sd.declarator = parseDeclarator();
    }

    // optional bit-field width
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":") {
        lex.next();
        sd.bitfieldWidth = parseConditionalExpression();
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
                size = parseConditionalExpression();
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
            bool isVariadic = false;

            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                // `...` before any named parameter is illegal in C.
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "...") {
                    wvmcc::Diagnostic d;
                    d.severity = wvmcc::Diagnostic::Severity::Error;
                    d.message = "'...' requires at least one named parameter";
                    d.span = lex.peek()->span;
                    diagnostics.push_back(std::move(d));
                    lex.next(); // consume to recover
                }
                // heuristics: if it starts with a type keyword or a typedef-name treat as parameter-type-list
                if (lex.peek() && (lex.peek()->kind() == TokenKind::Keyword || (lex.peek()->kind() == TokenKind::Identifier && typedef_names.count(lex.peek()->lexeme())))) {
                    hasParamTypeList = true;
                    while (true) {
                        auto pspecs = parseDeclarationSpecifiers();
                        Parameter param;
                        param.specifiers = pspecs;
                        if (lex.peek() && (lex.peek()->kind() == TokenKind::Identifier || (lex.peek()->kind() == TokenKind::Punctuator && (lex.peek()->lexeme() == "(" || lex.peek()->lexeme() == "*")) )) {
                            param.declarator = parseDeclarator();
                        } else {
                            param.declarator = nullptr; // abstract-declarator not implemented
                        }
                        params.push_back(param);
                        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
                            lex.next();
                            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "...") {
                                lex.next();
                                isVariadic = true;
                                if (lex.peek() && !(lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                                    wvmcc::Diagnostic d;
                                    d.severity = wvmcc::Diagnostic::Severity::Error;
                                    d.message = "'...' must be the last parameter";
                                    d.span = lex.peek()->span;
                                    diagnostics.push_back(std::move(d));
                                }
                                break;
                            }
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
            fn->function.isVariadic = isVariadic;
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
    // optional GCC __attribute__((...)) before the declaration-specifiers
    auto gnuAttrs = parseGnuAttributeSpecifierList();
    // gather specifiers (keywords like 'int', 'static', etc.)
    auto specs = parseDeclarationSpecifiers();
    // attributes may also appear between declaration-specifiers and declarator
    {
        auto more = parseGnuAttributeSpecifierList();
        gnuAttrs.insert(gnuAttrs.end(), std::make_move_iterator(more.begin()), std::make_move_iterator(more.end()));
    }

    // Handle _Static_assert (C 6.7.10): create a StaticAssert external node so
    // semantic checks can evaluate the constant-expression with TU context.
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
        auto expr = parseConditionalExpression();
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
        ExprPtr msgExpr = nullptr;
        if (lex.peek() && lex.peek()->kind() == TokenKind::StringLiteral) {
            auto tok = *lex.next();
            auto sl = make_ast<StringLiteral>();
            sl->span = tok.span;
            sl->value = tok.lexeme();
            sl->kind = Expr::Kind::String;
            msgExpr = sl;
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

        // build AST node for static assert and return as external declaration
        auto sa = make_ast<ExternalDecl::StaticAssert>();
        sa->span = expr ? expr->span : SourceSpan{};
        sa->expr = expr;
        sa->message = msgExpr;
        // Parser-level evaluation to preserve existing parser tests: evaluate constant-expression
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
            std::string tmsg = "static assertion failed";
            if (msgExpr && msgExpr->kind == Expr::Kind::String) {
                auto sl = std::dynamic_pointer_cast<StringLiteral>(msgExpr);
                if (sl) tmsg = std::string("static assertion failed: ") + sl->value;
            } else {
                tmsg = std::string("static assertion failed");
            }
            d.message = tmsg;
            if (expr) d.span = expr->span;
            diagnostics.push_back(std::move(d));
        }

        auto ext = make_ast<ExternalDecl>();
        ext->span = sa->span;
        ext->decl = std::static_pointer_cast<ExternalDecl::StaticAssert>(sa);
        return ext;
    }

    // Try to parse an optional declarator (may be null for declarations like "struct S;")
    DeclaratorPtr decl = nullptr;
    // Only attempt to parse a declarator if next token could start one
    if (lex.peek()) {
        if (lex.peek()->kind() == TokenKind::Identifier || (lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") || (lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "*")) {
            decl = parseDeclarator();
        }
    }
    // attributes may also appear after the declarator (before initializer or ';')
    {
        auto more = parseGnuAttributeSpecifierList();
        gnuAttrs.insert(gnuAttrs.end(), std::make_move_iterator(more.begin()), std::make_move_iterator(more.end()));
    }

    // Early constraint check: storage-class specifiers 'auto' and 'register' are invalid
    // in external declarations (C standard 6.9). Emit parser diagnostics (constraint).
    if (specs.hasStorage(StorageClass::Auto) || specs.hasStorage(StorageClass::Register)) {
        wvmcc::Diagnostic d;
        d.severity = wvmcc::Diagnostic::Severity::Error;
        if (specs.hasStorage(StorageClass::Auto)) d.message = "storage-class specifier 'auto' is not allowed in external declarations";
        else d.message = "storage-class specifier 'register' is not allowed in external declarations";
        // span: use declarator span if available, else current token
        if (decl && decl->span.begin.line) d.span = decl->span;
        else if (lex.peek()) d.span = lex.peek()->span;
        diagnostics.push_back(std::move(d));
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
            f->gnuAttributes = std::move(gnuAttrs);
            auto ext = make_ast_with_span<ExternalDecl>(f->span);
            ext->decl = f;

            // duplicate internal definition check for 'static' functions
            if (!name.empty() && specs.hasStorage(StorageClass::Static)) {
                auto it = internal_definitions.find(name);
                bool is_definitive = true; // function definition is definitive
                if (it != internal_definitions.end()) {
                    // if previous was definitive, this is a duplicate internal definition -> emit parser constraint diagnostic
                    if (it->second.second) {
                        wvmcc::Diagnostic dd;
                        dd.severity = wvmcc::Diagnostic::Severity::Error;
                        dd.message = "duplicate internal definition of '" + name + "' in translation unit";
                        dd.span = f->span;
                        diagnostics.push_back(std::move(dd));
                    }
                    it->second = std::make_pair(f->span, true);
                } else {
                    internal_definitions[name] = std::make_pair(f->span, true);
                }
            }

            return ext;
        } else {
                auto d = parseDeclaration(specs, decl);
                if (!d) return nullptr;
                d->gnuAttributes = std::move(gnuAttrs);
                // static/thread storage duration initializers must be constant (C 6.7.9 constraint 4)
                if (d->initializer.has_value() && (specs.hasStorage(StorageClass::Static) || specs.hasStorage(StorageClass::ThreadLocal))) {
                    // semantic check (constant initializer) moved to Semantic pass
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
        d->gnuAttributes = std::move(gnuAttrs);
        auto ext = make_ast_with_span<ExternalDecl>(d->span);
        ext->decl = d;
        return ext;
    }

    // otherwise, if we have a declarator (non-function) treat as declaration
    if (decl) {
        auto d = parseDeclaration(specs, decl);
        if (!d) return nullptr;
        d->gnuAttributes = std::move(gnuAttrs);
        // If this declaration has static/thread storage duration, its initializer
        // expressions must be constant expressions or string literals (C 6.7.9 constraint 4).
        if (d->initializer.has_value() && (specs.hasStorage(StorageClass::Static) || specs.hasStorage(StorageClass::ThreadLocal))) {
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
                // if previously definitive, emit duplicate internal definition error (constraint-level)
                if (it != internal_definitions.end() && it->second.second) {
                    wvmcc::Diagnostic dd;
                    dd.severity = wvmcc::Diagnostic::Severity::Error;
                    dd.message = "duplicate internal definition of '" + nm + "' in translation unit";
                    dd.span = d->span;
                    diagnostics.push_back(std::move(dd));
                    it->second = std::make_pair(d->span, true);
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

    // Extract params from the function-kind declarator in the chain
    for (auto cur = decl; cur; cur = cur->inner.has_value() ? *cur->inner : nullptr) {
        if (cur->kind == Declarator::Kind::Function) {
            f->params = cur->function.params;
            f->isVariadic = cur->function.isVariadic;
            break;
        }
    }

    // initialize per-function parsing state
    labels_in_current_function.clear();
    gotos_in_current_function.clear();
    stmt_context_stack.clear();
    current_function_specs = specs;
    // Walk the declarator chain: any Pointer layer between the outer Function
    // layer and the inner Identifier means the return type is a pointer.
    current_function_returns_pointer = false;
    for (auto cur = f->declarator; cur; cur = cur->inner.value_or(nullptr)) {
        if (cur->kind == Declarator::Kind::Pointer) {
            current_function_returns_pointer = true;
            break;
        }
        if (!cur->inner.has_value()) break;
    }
    f->body = parseCompoundBody();
    // validate gotos: each goto must name an existing label in this function
    // (defer reporting to Semantic pass)
    // clear current function speculative state
    current_function_specs.reset();
    current_function_returns_pointer = false;
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

    // Constraint C 6.7.2.2: a declaration (other than static_assert)
    // shall declare at least a declarator, a tag (struct/union with members),
    // or the members of an enumeration. Emit a parser-level diagnostic
    // if none of these are present.
    bool hasTagWithMembers = false;
    for (const auto &ts : decl->specifiers.typeSpecifiers) {
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
            if (ts.su) { hasTagWithMembers = true; break; }
        }
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
            if (ts.en) { hasTagWithMembers = true; break; }
        }
    }
    if (!decl->declarator && !hasTagWithMembers) {
        wvmcc::Diagnostic d;
        d.severity = wvmcc::Diagnostic::Severity::Error;
        d.message = "declaration must declare at least a declarator, a tag, or enum members";
        d.message += " (typeSpecifiers=" + std::to_string(decl->specifiers.typeSpecifiers.size()) + ")";
        d.span = decl->span;
        diagnostics.push_back(std::move(d));
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
    // Constraint C 6.7.2.2: require declarator/tag/enum-members for declarations
    bool hasTagWithMembers2 = false;
    for (const auto &ts : decl->specifiers.typeSpecifiers) {
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
            if (ts.su) { hasTagWithMembers2 = true; break; }
        }
        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Enum) {
            if (ts.en) { hasTagWithMembers2 = true; break; }
        }
    }
    if (!decl->declarator && !hasTagWithMembers2) {
        wvmcc::Diagnostic d;
        d.severity = wvmcc::Diagnostic::Severity::Error;
        d.message = "declaration must declare at least a declarator, a tag, or enum members";
        d.span = decl->span;
        diagnostics.push_back(std::move(d));
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
            auto returnExpr = parseExpression();
            // `return;` (no expression) leaves parseExpression with nullptr.
            // Don't wrap nullptr in the optional or downstream checks will
            // think we have an expression that's been silently lost.
            if (returnExpr) rs->value = returnExpr;
            if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
            // validate return vs function return type if available
            if (current_function_specs.has_value()) {
                bool funcVoid = false;
                for (auto &ts : current_function_specs->typeSpecifiers) {
                    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                        for (auto &st : ts.simple) {
                            if (st == DeclarationSpecifiers::SimpleTypeSpecifier::Void) funcVoid = true;
                        }
                    }
                }
                // `void *f(...)` returns a pointer-to-void, not void.
                if (current_function_returns_pointer) funcVoid = false;
                if (rs->value.has_value() && funcVoid) {
                    wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "return with expression in function returning void"; d.span = rs->span; diagnostics.push_back(std::move(d));
                }
                if (!rs->value.has_value() && !funcVoid) {
                    wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "return without expression in non-void function"; d.span = rs->span; diagnostics.push_back(std::move(d));
                }
            }
            auto bi = make_ast<BlockItem>();
            rs->kind = Stmt::Kind::Return;
            bi->item = std::static_pointer_cast<Stmt>(rs);
            body.push_back(bi);
            continue;
        }

        // case / default labels (only valid inside switch but we parse them here)
        if (p->kind() == TokenKind::Keyword && p->lexeme() == "case") {
            // consume 'case'
            lex.next();
            // parse constant/conditional expression
            ExprPtr val = parseConditionalExpression();
            // expect ':'
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":")) {
                if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ':' after case expression"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
            } else lex.next();
            // ensure we're inside a switch
            bool inSwitch = false;
            for (auto it = stmt_context_stack.rbegin(); it != stmt_context_stack.rend(); ++it) { if (*it == Stmt::Kind::Switch) { inSwitch = true; break; } }
            if (!inSwitch) {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "'case' label not within a switch"; if (p) d.span = p->span; diagnostics.push_back(std::move(d));
            }
            // parse the statement following the case label
            StmtPtr sub = parseStmt();
            auto cs = make_ast<CaseStmt>();
            cs->value = val;
            cs->stmt = sub;
            cs->kind = Stmt::Kind::Case;
            cs->span = val ? val->span : SourceSpan{};
            auto bi = make_ast<BlockItem>();
            bi->item = std::static_pointer_cast<Stmt>(cs);
            body.push_back(bi);
            continue;
        }

        if (p->kind() == TokenKind::Keyword && p->lexeme() == "default") {
            // ensure we're inside a switch
            bool inSwitch = false;
            for (auto it = stmt_context_stack.rbegin(); it != stmt_context_stack.rend(); ++it) { if (*it == Stmt::Kind::Switch) { inSwitch = true; break; } }
            if (!inSwitch) {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "'default' label not within a switch"; if (p) d.span = p->span; diagnostics.push_back(std::move(d));
            }
            lex.next();
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":")) {
                if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ':' after default"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
            } else lex.next();
            StmtPtr sub = parseStmt();
            auto ds = make_ast<DefaultStmt>();
            ds->stmt = sub;
            ds->kind = Stmt::Kind::Default;
            ds->span = sub ? sub->span : SourceSpan{};
            auto bi = make_ast<BlockItem>();
            bi->item = std::static_pointer_cast<Stmt>(ds);
            body.push_back(bi);
            continue;
        }

        // _Static_assert as block item (C17 §6.8.2)
        if (p->kind() == TokenKind::Keyword && p->lexeme() == "_Static_assert") {
            lex.next();
            if (!acceptPunct("(")) {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after _Static_assert";
                if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
                while (lex.peek() && !(lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";")) lex.next();
                if (lex.peek()) lex.next();
                continue;
            }
            auto expr = parseConditionalExpression();
            if (!(lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==",")) {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ',' in _Static_assert";
                if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
            } else { lex.next(); }
            ExprPtr msgExpr = nullptr;
            if (lex.peek() && lex.peek()->kind() == TokenKind::StringLiteral) {
                auto tok = *lex.next();
                auto sl = make_ast<StringLiteral>(); sl->span = tok.span; sl->value = tok.lexeme(); sl->kind = Expr::Kind::String;
                msgExpr = sl;
            } else {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected string literal in _Static_assert";
                if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
            }
            if (!(lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==")")) {
                if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after _Static_assert"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
            } else { lex.next(); }
            if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
            auto sa = make_ast<ExternalDecl::StaticAssert>();
            sa->span = expr ? expr->span : SourceSpan{};
            sa->expr = expr;
            sa->message = msgExpr;
            auto val = ConstExprEvaluator::evalIntegerConstantExpr(expr);
            if (!val.has_value()) {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "_Static_assert requires an integer constant expression";
                if (expr) d.span = expr->span; diagnostics.push_back(std::move(d));
            } else if (*val == 0) {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error;
                std::string tmsg = "static assertion failed";
                if (msgExpr && msgExpr->kind == Expr::Kind::String) { auto sl = std::dynamic_pointer_cast<StringLiteral>(msgExpr); if (sl) tmsg = std::string("static assertion failed: ") + sl->value; }
                d.message = tmsg; if (expr) d.span = expr->span; diagnostics.push_back(std::move(d));
            }
            auto bi = make_ast<BlockItem>();
            bi->item = std::static_pointer_cast<ExternalDecl::StaticAssert>(sa);
            body.push_back(bi);
            continue;
        }

        if (p->kind() == TokenKind::Keyword || p->kind() == TokenKind::Identifier) {
            // attempt to parse declaration specifiers; if none found, treat as statement
            auto specs = parseDeclarationSpecifiers();
            if (!specs.empty()) {
                DeclaratorPtr maybeDeclr = nullptr;
                if (lex.peek() && (lex.peek()->kind() == TokenKind::Identifier || (lex.peek()->kind() == TokenKind::Punctuator && (lex.peek()->lexeme() == "(" || lex.peek()->lexeme() == "*")))) {
                    maybeDeclr = parseDeclarator();
                }
                auto decl = parseDeclaration(specs, maybeDeclr);
                auto bi = make_ast<BlockItem>();
                bi->item = decl;
                // C 6.7.9 constraint 5: if declaration has block scope and the identifier has
                // external linkage, the declaration shall have no initializer. Block-scope
                // `static` gives the identifier no linkage (C 6.2.2p6), so initializers are
                // permitted (and are evaluated once on first call).
                if (decl && decl->initializer.has_value() && specs.hasStorage(StorageClass::Extern)) {
                    wvmcc::Diagnostic diag;
                    diag.severity = wvmcc::Diagnostic::Severity::Error;
                    diag.message = "declaration at block scope with external linkage shall not have an initializer";
                    diag.span = decl->span;
                    diagnostics.push_back(std::move(diag));
                }
                body.push_back(bi);
                continue;
            }
            // if we started with a keyword and no declaration specifiers were parsed,
            // it is likely a statement keyword (if/switch/for/while/return/etc.).
            if (p->kind() == TokenKind::Keyword) {
                StmtPtr st = parseStmt();
                auto bi = make_ast<BlockItem>();
                bi->item = st;
                body.push_back(bi);
                continue;
            }

            // fall through to statement handling when specs was empty (identifiers handled below)
        }

        // handle labeled-statement: identifier ':' statement
        if (p->kind() == TokenKind::Identifier) {
            // lookahead for ':'
            auto idtok = *lex.next();
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":") {
                // it's a label
                lex.next(); // consume ':'
                // parse the sub-statement
                StmtPtr sub = parseStmt();
                auto ls = make_ast<LabelStmt>();
                ls->name = idtok.lexeme();
                ls->stmt = sub;
                ls->span = idtok.span;
                ls->kind = Stmt::Kind::Label;
                // uniqueness check within function: record labels, semantic will report duplicates
                labels_in_current_function.insert(ls->name);
                auto bi = make_ast<BlockItem>();
                bi->item = std::static_pointer_cast<Stmt>(ls);
                body.push_back(bi);
                continue;
            } else {
                // not a label: we consumed an identifier that may start an expression -> rebuild as identifier expression
                auto idexpr = make_ast<IdentifierExpr>();
                idexpr->span = idtok.span;
                idexpr->name = idtok.lexeme();
                idexpr->kind = Expr::Kind::Ident;
                // Continue parsing as a full expression with idexpr as the already-parsed LHS.
                // applyPostfixSuffix handles calls, indexing, member access; then we continue
                // through the assignment/binary hierarchy via parseAssignmentExpression-like logic.
                // We approximate by applying postfix then checking for assignment operators.
                ExprPtr lhs = applyPostfixSuffix(idexpr);
                // Handle assignment: lhs = rhs
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
                    std::string op = lex.peek()->lexeme();
                    if (op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=" ||
                        op == "%=" || op == "<<=" || op == ">>=" || op == "&=" || op == "|=" || op == "^=") {
                        lex.next();
                        ExprPtr rhs = parseAssignmentExpression();
                        auto be = make_ast<BinaryExpr>();
                        be->op = op; be->lhs = lhs; be->rhs = rhs;
                        be->kind = Expr::Kind::Binary; be->span = lhs->span;
                        lhs = be;
                    } else if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
                               op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=" ||
                               op == "&&" || op == "||" || op == "&" || op == "|" || op == "^" ||
                               op == "<<" || op == ">>" || op == ",") {
                        // binary op: parse rhs as assignment-expression then build binary node
                        lex.next();
                        ExprPtr rhs = parseAssignmentExpression();
                        auto be = make_ast<BinaryExpr>();
                        be->op = op; be->lhs = lhs; be->rhs = rhs;
                        be->kind = Expr::Kind::Binary; be->span = lhs->span;
                        lhs = be;
                    }
                }
                ExprPtr finalExpr = lhs;
                if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
                auto es = make_ast<ExprStmt>();
                es->expr = finalExpr;
                es->kind = Stmt::Kind::Expr;
                es->span = finalExpr ? finalExpr->span : SourceSpan{};
                auto bi = make_ast<BlockItem>();
                bi->item = std::static_pointer_cast<Stmt>(es);
                body.push_back(bi);
                continue;
            }
        }

        // default: expression statement
        auto expr = parseExpression();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        auto es = make_ast<ExprStmt>();
        es->expr = expr;
        es->kind = Stmt::Kind::Expr;
        es->span = expr ? expr->span : SourceSpan{};
        auto bi = make_ast<BlockItem>();
        bi->item = std::static_pointer_cast<Stmt>(es);
        body.push_back(bi);
    }
    return body;
}

StmtPtr Parser::parseStmt() {
    auto p = lex.peek();
    if (!p) return nullptr;
    // compound-statement
    if (p->kind() == TokenKind::Punctuator && p->lexeme() == "{") {
        // consume '{'
        lex.next();
        auto cs = make_ast<CompoundStmt>();
        cs->items = parseCompoundBody();
        cs->kind = Stmt::Kind::Compound;
        return std::static_pointer_cast<Stmt>(cs);
    }

    // empty statement ';'
    if (p->kind() == TokenKind::Punctuator && p->lexeme() == ";") {
        lex.next();
        auto s = make_ast<Stmt>();
        s->kind = Stmt::Kind::Empty;
        return s;
    }

    // return statement
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "return") {
        lex.next();
        auto rs = make_ast<ReturnStmt>();
        rs->span = p->span;
        rs->value = parseExpression();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        rs->kind = Stmt::Kind::Return;
        return std::static_pointer_cast<Stmt>(rs);
    }

    // if statement
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "if") {
        lex.next();
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after if"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else lex.next();
        ExprPtr cond = parseExpression();
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
        StmtPtr thenS = parseStmt();
        std::optional<StmtPtr> elseS;
        if (lex.peek() && lex.peek()->kind() == TokenKind::Keyword && lex.peek()->lexeme() == "else") {
            lex.next();
            elseS = parseStmt();
        }
        auto ifs = make_ast<IfStmt>();
        ifs->cond = cond;
        ifs->thenStmt = thenS;
        ifs->elseStmt = elseS;
        ifs->kind = Stmt::Kind::If;
        ifs->span = cond ? cond->span : SourceSpan{};
        return std::static_pointer_cast<Stmt>(ifs);
    }

    // switch statement
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "switch") {
        lex.next();
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after switch"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else lex.next();
        ExprPtr cond = parseExpression();
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
        // push switch context so case/default checks work
        stmt_context_stack.push_back(Stmt::Kind::Switch);
        StmtPtr body = parseStmt();
        stmt_context_stack.pop_back();
        auto ss = make_ast<SwitchStmt>();
        ss->cond = cond;
        ss->body = body;
        ss->kind = Stmt::Kind::Switch;
        ss->span = cond ? cond->span : SourceSpan{};
        return std::static_pointer_cast<Stmt>(ss);
    }

    // while
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "while") {
        lex.next();
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after while"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else lex.next();
        ExprPtr cond = parseExpression();
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
        stmt_context_stack.push_back(Stmt::Kind::While);
        StmtPtr body = parseStmt();
        stmt_context_stack.pop_back();
        auto ws = make_ast<WhileStmt>();
        ws->cond = cond;
        ws->body = body;
        ws->kind = Stmt::Kind::While;
        ws->span = cond ? cond->span : SourceSpan{};
        return std::static_pointer_cast<Stmt>(ws);
    }

    // do-while
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "do") {
        lex.next();
        stmt_context_stack.push_back(Stmt::Kind::DoWhile);
        StmtPtr body = parseStmt();
        stmt_context_stack.pop_back();
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Keyword && lex.peek()->lexeme() == "while")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected 'while' after do body"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else lex.next();
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after while"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else lex.next();
        ExprPtr cond = parseExpression();
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        auto ds = make_ast<DoWhileStmt>();
        ds->body = body;
        ds->cond = cond;
        ds->kind = Stmt::Kind::DoWhile;
        ds->span = cond ? cond->span : SourceSpan{};
        return std::static_pointer_cast<Stmt>(ds);
    }

    // for
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "for") {
        lex.next();
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after for"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else lex.next();
        auto fs = make_ast<ForStmt>();
        // clause-1: either declaration or expressionopt
        if (lex.peek() && (lex.peek()->kind() == TokenKind::Keyword || lex.peek()->kind() == TokenKind::Identifier)) {
            auto specs = parseDeclarationSpecifiers();
            if (!specs.empty()) {
                std::string name;
                if (lex.peek() && lex.peek()->kind() == TokenKind::Identifier) { name = lex.peek()->lexeme(); lex.next(); }
                auto decl = parseDeclaration(specs, name);
                auto bi = make_ast<BlockItem>();
                bi->item = decl;
                fs->init = bi;
            } else {
                // expressionopt
                if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ";")) {
                    auto expr = parseExpression();
                    auto es = make_ast<ExprStmt>(); es->expr = expr; es->kind = Stmt::Kind::Expr; es->span = expr?expr->span:SourceSpan{};
                    auto bi = make_ast<BlockItem>(); bi->item = std::static_pointer_cast<Stmt>(es);
                    fs->init = bi;
                }
                if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") {
                    lex.next();
                }
            }
        } else {
            // empty init (immediately semicolon expected)
            if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        }

        // expression-2 (condition) optional
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ";")) {
            fs->cond = parseExpression();
        }
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();

        // expression-3 (step) optional
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
            fs->step = parseExpression();
        }
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==")") lex.next();
        stmt_context_stack.push_back(Stmt::Kind::For);
        fs->body = parseStmt();
        stmt_context_stack.pop_back();
        fs->kind = Stmt::Kind::For;
        fs->span = fs->body ? fs->body->span : SourceSpan{};
        return std::static_pointer_cast<Stmt>(fs);
    }

    // break
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "break") {
        lex.next();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        // ensure we're inside a loop or switch
        bool ok = false;
        for (auto it = stmt_context_stack.rbegin(); it != stmt_context_stack.rend(); ++it) {
            if (*it == Stmt::Kind::While || *it == Stmt::Kind::For || *it == Stmt::Kind::DoWhile || *it == Stmt::Kind::Switch) { ok = true; break; }
        }
        if (!ok) {
            wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "'break' not inside loop or switch"; if (p) d.span = p->span; diagnostics.push_back(std::move(d));
        }
        auto b = make_ast<BreakStmt>(); b->kind = Stmt::Kind::Break; return std::static_pointer_cast<Stmt>(b);
    }

    // continue
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "continue") {
        lex.next();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        bool ok = false;
        for (auto it = stmt_context_stack.rbegin(); it != stmt_context_stack.rend(); ++it) {
            if (*it == Stmt::Kind::While || *it == Stmt::Kind::For || *it == Stmt::Kind::DoWhile) { ok = true; break; }
        }
        if (!ok) {
            wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "'continue' not inside a loop"; if (p) d.span = p->span; diagnostics.push_back(std::move(d));
        }
        auto c = make_ast<ContinueStmt>(); c->kind = Stmt::Kind::Continue; return std::static_pointer_cast<Stmt>(c);
    }

    // goto
    if (p->kind() == TokenKind::Keyword && p->lexeme() == "goto") {
        lex.next();
        std::string label;
        SourceSpan labspan{};
        if (lex.peek() && lex.peek()->kind() == TokenKind::Identifier) { label = lex.peek()->lexeme(); labspan = lex.peek()->span; lex.next(); }
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        // record goto for later validation
        gotos_in_current_function.push_back({label, labspan});
        auto g = make_ast<GotoStmt>(); g->label = label; g->kind = Stmt::Kind::Goto; return std::static_pointer_cast<Stmt>(g);
    }

    // fallback: expression statement
    {
        auto expr = parseExpression();
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()==";") lex.next();
        auto es = make_ast<ExprStmt>();
        es->expr = expr;
        es->span = expr ? expr->span : SourceSpan{};
        es->kind = Stmt::Kind::Expr;
        return std::static_pointer_cast<Stmt>(es);
    }
}



// Simple primary expression parser: integer, identifier, string
ExprPtr Parser::parsePrimary() {
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
    if (t->kind() == TokenKind::CharacterConstant) {
        auto tok = *lex.next();
        auto cl = make_ast<CharLiteral>();
        cl->span = tok.span;
        // Extract the decoded numeric value out of the CharacterToken variant.
        if (auto* ct = std::get_if<CharacterToken>(&tok.v)) {
            cl->value = (char)(ct->info.value & 0xff);
        }
        cl->kind = Expr::Kind::Char;
        return cl;
    }
    if (t->kind() == TokenKind::FloatingConstant) {
        auto tok = *lex.next();
        auto fl = make_ast<FloatLiteral>();
        fl->span = tok.span;
        fl->raw = tok.lexeme();
        // Detect suffix (f/F/l/L). Strip before stod.
        bool isFloat = false;
        std::string body = fl->raw;
        if (!body.empty()) {
            char last = body.back();
            if (last == 'f' || last == 'F') { isFloat = true; body.pop_back(); }
            else if (last == 'l' || last == 'L') { body.pop_back(); }
        }
        try { fl->value = std::stod(body); } catch (...) { fl->value = 0.0; }
        fl->isFloat = isFloat;
        fl->kind = Expr::Kind::Float;
        return fl;
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
    // Generic selection: _Generic ( assignment-expression , generic-assoc-list )
    if (t->kind() == TokenKind::Keyword && t->lexeme() == "_Generic") {
        lex.next();
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(")) {
            wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after _Generic"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
            return nullptr;
        }
        // consume '('
        lex.next();
        // parse controlling assignment-expression
        ExprPtr ctrl = parseAssignmentExpression();
        if (!ctrl) {
            wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected controlling expression in _Generic"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
        }
        // expect comma
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",")) {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ',' after controlling expression in _Generic"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        } else lex.next();

        std::vector<GenericAssociation> assocs;
        bool defaultSeen = false;
        // parse generic-assoc-list: one or more generic-association separated by ','
        while (true) {
            if (!(lex.peek())) break;
            // check for default : assignment-expression
            if (lex.peek()->kind() == TokenKind::Keyword && lex.peek()->lexeme() == "default") {
                lex.next();
                if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":")) {
                    if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ':' after default in _Generic"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
                } else lex.next();
                ExprPtr ae = parseAssignmentExpression();
                GenericAssociation ga; ga.isDefault = true; ga.type = nullptr; ga.expr = ae;
                if (defaultSeen) {
                    wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "duplicate default generic association in _Generic"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
                }
                defaultSeen = true;
                assocs.push_back(std::move(ga));
            } else {
                // attempt to parse type-name using declaration specifiers heuristic
                bool isTypeNameStart = false;
                if (lex.peek()) {
                    auto u = lex.peek();
                    if (u->kind() == TokenKind::Keyword) {
                        static const std::unordered_set<std::string> types = {"void","char","short","int","long","float","double","signed","unsigned","_Bool","_Complex","_Imaginary","struct","union","enum"};
                        if (types.count(u->lexeme())) isTypeNameStart = true;
                    } else if (u->kind() == TokenKind::Identifier) {
                        if (typedef_names.count(u->lexeme())) isTypeNameStart = true;
                    }
                }
                if (!isTypeNameStart) {
                    // error: expected type-name or default
                    wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected type name or 'default' in _Generic association"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
                    // try to recover by consuming token
                    lex.next();
                } else {
                    auto specs = parseDeclarationSpecifiers();
                    // build minimal TypeNode from specs
                    auto tn = make_ast<TypeNode>();
                    if (!specs.typeSpecifiers.empty()) {
                        auto &ts = specs.typeSpecifiers.front();
                        if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                            tn->kind = TypeNode::Kind::Builtin;
                            tn->simple = ts.simple;
                        } else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Other) {
                            tn->kind = TypeNode::Kind::Builtin;
                            tn->text = ts.text;
                        } else {
                            tn->kind = TypeNode::Kind::Builtin;
                            tn->text = "type";
                        }
                    } else { tn->kind = TypeNode::Kind::Builtin; tn->text = "type"; }
                    // expect ':'
                    if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":")) {
                        if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ':' after type name in _Generic association"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
                    } else lex.next();
                    ExprPtr ae = parseAssignmentExpression();
                    GenericAssociation ga; ga.isDefault = false; ga.type = tn; ga.expr = ae;
                    assocs.push_back(std::move(ga));
                }
            }

            // if next is ',' consume and continue; if ')' break; else try to continue
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
                lex.next();
                // allow trailing comma before ')'
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") break;
                continue;
            }
            break;
        }

        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") {
            lex.next();
        } else {
            if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' to close _Generic"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
        }

        auto ge = make_ast<GenericSelectionExpr>();
        ge->controlling = ctrl;
        ge->assocs = std::move(assocs);
        ge->kind = Expr::Kind::GenericSelection;
        ge->span = ctrl ? ctrl->span : SourceSpan{};
        return ge;
    }
    // parenthesized expressions and compound-literals
    if (t->kind() == TokenKind::Punctuator && t->lexeme() == "(") {
        // consume '('
        lex.next();
        // heuristics: if it looks like a type-name, parse a possible compound-literal
        bool isTypeNameStart = false;
        if (lex.peek()) {
            auto u = lex.peek();
            if (u->kind() == TokenKind::Keyword) {
                static const std::unordered_set<std::string> types = {"void","char","short","int","long","float","double","signed","unsigned","_Bool","_Complex","_Imaginary","struct","union","enum"};
                if (types.count(u->lexeme())) isTypeNameStart = true;
            } else if (u->kind() == TokenKind::Identifier) {
                if (typedef_names.count(u->lexeme())) isTypeNameStart = true;
            }
        }
        if (isTypeNameStart) {
            // parse type-name heuristically
            auto specs = parseDeclarationSpecifiers();
            // expect ')'
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after type name"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
            } else lex.next();

            // if a '{' follows, this is a compound-literal
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
                auto clit = make_ast<CompoundLiteral>();
                auto tn = make_ast<TypeNode>();
                if (!specs.typeSpecifiers.empty()) {
                    auto &ts = specs.typeSpecifiers.front();
                    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) { tn->kind = TypeNode::Kind::Builtin; tn->simple = ts.simple; }
                    else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su) { tn->kind = (ts.su->kind == StructOrUnionSpecifier::Kind::Struct) ? TypeNode::Kind::Struct : TypeNode::Kind::Union; tn->su = ts.su; }
                    else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Other) { tn->kind = TypeNode::Kind::Builtin; tn->text = ts.text; }
                    else { tn->kind = TypeNode::Kind::Builtin; tn->text = "type"; }
                } else { tn->kind = TypeNode::Kind::Builtin; tn->text = "type"; }
                clit->type = tn;
                clit->init = parseInitializer();
                clit->kind = Expr::Kind::CompoundLiteral;
                clit->span = clit->init ? clit->init->span : SourceSpan{};
                return clit;
            }
            // otherwise treat as parenthesized expression: parse assignment-expression
            ExprPtr inner = parseAssignmentExpression();
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
            return inner;
        }

        // not a type-name: parse a parenthesized expression
        ExprPtr inner = parseAssignmentExpression();
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
        return inner;
    }

    // No valid primary here. Return nullptr so callers (parseExpression,
    // parseAssignmentExpression, etc.) can report "no expression" — letting
    // `return;` correctly produce a value-less return, for instance.
    // Statement-terminator punctuators in particular must NEVER be consumed
    // by the expression parser.
    if (auto tok = lex.peek()) {
        if (tok->kind() == TokenKind::Punctuator) {
            const auto& lx = tok->lexeme();
            if (lx == ";" || lx == ")" || lx == "}" || lx == "]" || lx == ",") {
                return nullptr;
            }
        }
    }
    // Last-resort fallback for genuinely unrecognized tokens — produce a
    // synthetic identifier so the rest of the parser can recover. Skipping
    // this would drop user code from the AST silently.
    auto tok = *lex.next();
    auto id = make_ast<IdentifierExpr>();
    id->span = tok.span;
    id->name = tok.lexeme();
    id->kind = Expr::Kind::Ident;
    return id;
}

// Parse assignment-expression (right-associative for assignment operators)
ExprPtr Parser::parseAssignmentExpression() {
    // parse conditional-expression first (to support ternary operator as lhs)
    ExprPtr lhs = parseConditionalExpression();
    if (!lhs) return nullptr;

    // check for assignment operators
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
        std::string op = lex.peek()->lexeme();
        // multi-char operators: check two-char forms first
        if (op == "=" || op == "*=" || op == "/=" || op == "%=" || op == "+=" || op == "-=" || op == "<<=" || op == ">>=" || op == "&=" || op == "^=" || op == "|=") {
            // consume operator
            lex.next();
            // parse right-hand side as assignment-expression (right-associative)
            ExprPtr rhs = parseAssignmentExpression();
            auto be = make_ast<BinaryExpr>();
            be->op = op;
            be->lhs = lhs;
            be->rhs = rhs;
            be->kind = Expr::Kind::Binary;
            // set span from lhs to rhs if available
            be->span = lhs->span;
            if (rhs) be->span.end = rhs->span.end;
            return be;
        }
    }
    return lhs;
}

// expression: assignment-expression (',' assignment-expression)*
ExprPtr Parser::parseExpression() {
    ExprPtr lhs = parseAssignmentExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
        // consume comma
        lex.next();
        ExprPtr rhs = parseAssignmentExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = ",";
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// NOTE: `parseExpr()` was removed; use `parseAssignmentExpression()` or
// `parseConditionalExpression()` explicitly where needed.

// Parse unary expressions per C grammar (partial):
// Handles prefix ++/--, unary-operator cast-expression, sizeof, and _Alignof
ExprPtr Parser::parseUnaryExpression() {
    auto t = lex.peek();
    if (!t) return nullptr;

    // prefix ++ / --
    if (t->kind() == TokenKind::Punctuator) {
        std::string op = t->lexeme();
        if (op == "++" || op == "--") {
            lex.next();
            auto ue = make_ast<UnaryExpr>();
            ue->op = op;
            ue->rhs = parseUnaryExpression();
            ue->kind = Expr::Kind::Unary;
            ue->span = ue->rhs ? ue->rhs->span : t->span;
            return ue;
        }
        // unary-operator cast-expression: & * + - ~ !
        if (op == "&" || op == "*" || op == "+" || op == "-" || op == "~" || op == "!") {
            lex.next();
            auto ue = make_ast<UnaryExpr>();
            ue->op = op;
            ue->rhs = parseCastExpression();
            ue->kind = Expr::Kind::Unary;
            ue->span = ue->rhs ? ue->rhs->span : t->span;
            return ue;
        }
    }

    // keywords: sizeof, _Alignof
    if (t->kind() == TokenKind::Keyword) {
        std::string kw = t->lexeme();
        if (kw == "sizeof") {
            lex.next();
            // sizeof unary-expression or sizeof(type-name)
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") {
                // consume '('
                lex.next();
                // heuristic: next token starts a type-name?
                bool isType = false;
                if (lex.peek()) {
                    auto u = lex.peek();
                    if (u->kind() == TokenKind::Keyword) {
                        static const std::unordered_set<std::string> types = {"void","char","short","int","long","float","double","signed","unsigned","_Bool","_Complex","_Imaginary","struct","union","enum"};
                        if (types.count(u->lexeme())) isType = true;
                    } else if (u->kind() == TokenKind::Identifier) {
                        if (typedef_names.count(u->lexeme())) isType = true;
                    }
                }
                if (isType) {
                    auto specs = parseDeclarationSpecifiers();
                    if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                        if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after type name in sizeof"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
                    } else lex.next();
                    auto se = make_ast<SizeofExpr>();
                    se->type = make_ast<TypeNode>();
                    se->type.value()->kind = TypeNode::Kind::Builtin; se->type.value()->text = "type"; // minimal
                    se->expr = nullptr;
                    se->kind = Expr::Kind::Sizeof;
                    se->span = t->span;
                    return se;
                } else {
                    // sizeof ( expression )
                    ExprPtr inner = parseAssignmentExpression();
                    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
                    auto se = make_ast<SizeofExpr>();
                    se->type = std::nullopt;
                    se->expr = inner;
                    se->kind = Expr::Kind::Sizeof;
                    se->span = t->span;
                    return se;
                }
            }
            // sizeof unary-expression
            auto se = make_ast<SizeofExpr>();
            se->type = std::nullopt;
            se->expr = parseUnaryExpression();
            se->kind = Expr::Kind::Sizeof;
            se->span = t->span;
            return se;
        }

        if (kw == "_Alignof") {
            lex.next();
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(")) {
                wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected '(' after _Alignof"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
                return nullptr;
            }
            lex.next();
            auto specs = parseDeclarationSpecifiers();
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                if (lex.peek()) { wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after type name in _Alignof"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d)); }
            } else lex.next();
            auto ae = make_ast<AlignOfExpr>();
            ae->type = make_ast<TypeNode>();
            ae->type->kind = TypeNode::Kind::Builtin; ae->type->text = "type";
            // build a simple textual representation of the parsed type-specifiers
            auto makeTypeText = [&](const DeclarationSpecifiers &specs)->std::string {
                std::string out;
                for (const auto &ts : specs.typeSpecifiers) {
                    if (!out.empty()) out += " ";
                    switch (ts.kind) {
                        case DeclarationSpecifiers::TypeSpecifier::Kind::Simple: {
                            bool first = true;
                            for (auto st : ts.simple) {
                                if (!first) out += " ";
                                first = false;
                                using S = DeclarationSpecifiers::SimpleTypeSpecifier;
                                switch (st) {
                                    case S::Void: out += "void"; break;
                                    case S::Char: out += "char"; break;
                                    case S::Short: out += "short"; break;
                                    case S::Int: out += "int"; break;
                                    case S::Long: out += "long"; break;
                                    case S::Float: out += "float"; break;
                                    case S::Double: out += "double"; break;
                                    case S::Signed: out += "signed"; break;
                                    case S::Unsigned: out += "unsigned"; break;
                                    case S::Bool: out += "_Bool"; break;
                                    case S::Complex: out += "_Complex"; break;
                                    case S::Imaginary: out += "_Imaginary"; break;
                                    default: break;
                                }
                            }
                            break;
                        }
                        case DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion:
                            out += (ts.su && ts.su->kind == StructOrUnionSpecifier::Kind::Struct) ? "struct" : "union";
                            if (ts.su && ts.su->name) { out += " "; out += *ts.su->name; }
                            break;
                        case DeclarationSpecifiers::TypeSpecifier::Kind::TypedefName:
                            out += ts.text; break;
                        case DeclarationSpecifiers::TypeSpecifier::Kind::Enum:
                            out += "enum"; if (ts.en && ts.en->name) { out += " "; out += *ts.en->name; } break;
                        case DeclarationSpecifiers::TypeSpecifier::Kind::Atomic:
                            out += "_Atomic"; break;
                        case DeclarationSpecifiers::TypeSpecifier::Kind::Other:
                            out += ts.text; break;
                    }
                }
                return out;
            };
            ae->typeText = makeTypeText(specs);
            ae->kind = Expr::Kind::AlignOf;
            ae->span = t->span;
            return ae;
        }
    }

    // fallback: postfix-expression
    return parsePostfixExpression();
}

// Parse postfix-expression including indexing, calls, member access, postfix ++/--
ExprPtr Parser::applyPostfixSuffix(ExprPtr lhs) {
    while (true) {
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator)) break;
        std::string p = lex.peek()->lexeme();

        if (p == "[") {
            lex.next();
            ExprPtr idx = parseAssignmentExpression();
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "]") lex.next();
            auto ie = make_ast<IndexExpr>();
            ie->base = lhs; ie->index = idx; ie->kind = Expr::Kind::Index; ie->span = lhs->span;
            if (idx) ie->span.end = idx->span.end;
            lhs = ie; continue;
        }
        if (p == "(") {
            // Special-case __builtin_va_arg(expr, type-name): second arg is a
            // type-name, not an expression. Build a CallExpr with vaArgType
            // populated so codegen can size/load the slot correctly.
            bool isVaArg = false;
            if (lhs && lhs->kind == Expr::Kind::Ident) {
                const auto& id = static_cast<const IdentifierExpr&>(*lhs);
                if (id.name == "__builtin_va_arg") isVaArg = true;
            }
            if (isVaArg) {
                lex.next(); // '('
                std::vector<ExprPtr> args;
                ExprPtr apExpr = parseAssignmentExpression();
                args.push_back(apExpr);
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") {
                    lex.next();
                } else {
                    wvmcc::Diagnostic d;
                    d.severity = wvmcc::Diagnostic::Severity::Error;
                    d.message = "__builtin_va_arg expects (va_list, type-name)";
                    if (lex.peek()) d.span = lex.peek()->span;
                    diagnostics.push_back(std::move(d));
                }
                // Parse the type-name as declaration-specifiers + optional abstract declarator.
                auto vaSpecs = parseDeclarationSpecifiers();
                bool sawPointer = false;
                while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "*") {
                    lex.next();
                    sawPointer = true;
                    // skip qualifiers after '*'
                    while (lex.peek() && lex.peek()->kind() == TokenKind::Keyword) {
                        std::string qk = lex.peek()->lexeme();
                        if (qk == "const" || qk == "volatile" || qk == "restrict" || qk == "_Atomic") {
                            lex.next();
                        } else break;
                    }
                }
                if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") {
                    lex.next();
                }
                // Build a TypeNode from the parsed specifiers.
                auto tn = make_ast<TypeNode>();
                if (!vaSpecs.typeSpecifiers.empty()
                    && vaSpecs.typeSpecifiers[0].kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                    tn->kind = TypeNode::Kind::Builtin;
                    tn->simple = vaSpecs.typeSpecifiers[0].simple;
                } else {
                    tn->kind = TypeNode::Kind::Builtin;
                }
                if (sawPointer) {
                    auto ptr = make_ast<TypeNode>();
                    ptr->kind = TypeNode::Kind::Pointer;
                    ptr->pointee = tn;
                    tn = ptr;
                }
                auto ce = make_ast<CallExpr>();
                ce->callee = lhs;
                ce->args = std::move(args);
                ce->vaArgType = tn;
                ce->kind = Expr::Kind::Call;
                ce->span = lhs->span;
                lhs = ce;
                continue;
            }

            lex.next();
            std::vector<ExprPtr> args;
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                while (true) {
                    ExprPtr a = parseAssignmentExpression();
                    args.push_back(a);
                    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ",") { lex.next(); continue; }
                    break;
                }
            }
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
            auto ce = make_ast<CallExpr>();
            ce->callee = lhs; ce->args = std::move(args); ce->kind = Expr::Kind::Call; ce->span = lhs->span;
            lhs = ce; continue;
        }
        if (p == "." || p == "->") {
            bool isArrow = (p == "->"); lex.next();
            std::string member;
            if (lex.peek() && lex.peek()->kind() == TokenKind::Identifier) { member = lex.peek()->lexeme(); lex.next(); }
            auto me = make_ast<MemberExpr>();
            me->base = lhs; me->member = member; me->isArrow = isArrow; me->kind = Expr::Kind::Member; me->span = lhs->span;
            lhs = me; continue;
        }
        if (p == "++" || p == "--") {
            lex.next();
            auto pe = make_ast<PostfixUnaryExpr>();
            pe->op = (p == "++") ? PostfixUnaryExpr::Op::Inc : PostfixUnaryExpr::Op::Dec;
            pe->base = lhs; pe->kind = Expr::Kind::PostfixUnary; pe->span = lhs->span;
            lhs = pe; continue;
        }
        break;
    }
    return lhs;
}

ExprPtr Parser::parsePostfixExpression() {
    // start with a primary expression
    ExprPtr lhs = parsePrimary();
    if (!lhs) return nullptr;
    return applyPostfixSuffix(lhs);
}


// Parse conditional-expression: logical-or or '?:' ternary form
ExprPtr Parser::parseConditionalExpression() {
    // Parse a conditional-expression per C 6.5.15:
    //   conditional-expression: logical-OR-expression
    //                         | logical-OR-expression ? expression : conditional-expression
    ExprPtr lhs = parseLogicalOrExpression();
    if (!lhs) return nullptr;

    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "?") {
        // consume '?'
        lex.next();
        // parse second operand (expression)
        ExprPtr thenExpr = parseAssignmentExpression();

        // expect ':'
        if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ":")) {
            wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ':' in conditional expression"; if (lex.peek()) d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
        } else {
            lex.next();
        }

        // parse third operand (conditional-expression) — right-associative
        ExprPtr elseExpr = parseConditionalExpression();

        auto te = make_ast<TernaryExpr>();
        te->cond = lhs;
        te->thenExpr = thenExpr;
        te->elseExpr = elseExpr;
        te->kind = Expr::Kind::Ternary;
        te->span = lhs->span;
        if (elseExpr) te->span.end = elseExpr->span.end;
        else if (thenExpr) te->span.end = thenExpr->span.end;

        // NOTE: Semantic type checks required by C 6.5.15 (scalar condition, operand compatibility)
        // cannot be fully performed in the parser without a type system. Defer to semantic phase.
        if (!thenExpr || !elseExpr) {
            wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "incomplete conditional expression"; if (te->span.end.line==0) { if (lex.peek()) d.span = lex.peek()->span; } else d.span = te->span; diagnostics.push_back(std::move(d));
        }

        return te;
    }
    return lhs;
}

// logical-AND-expression: parse left-associative series of '&&' operations.
// For now operands are parsed with `parseAssignmentExpression()`; when the
// full expression grammar is available this should be adjusted to use the
// appropriate lower-precedence parsers (inclusive-or, etc.).
ExprPtr Parser::parseLogicalAndExpression() {
    ExprPtr lhs = parseInclusiveOrExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "&&") {
        // consume '&&'
        auto opTok = *lex.next();
        ExprPtr rhs = parseInclusiveOrExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = "&&";
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// logical-OR-expression: parse left-associative series of '||' operations.
ExprPtr Parser::parseLogicalOrExpression() {
    ExprPtr lhs = parseLogicalAndExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "||") {
        // consume '||'
        auto opTok = *lex.next();
        ExprPtr rhs = parseLogicalAndExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = "||";
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// exclusive-OR-expression: left-associative '^' using lower-level operands
ExprPtr Parser::parseExclusiveOrExpression() {
    ExprPtr lhs = parseAndExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "^") {
        lex.next();
        ExprPtr rhs = parseAndExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = "^";
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// inclusive-OR-expression: left-associative '|' using exclusive-or as operand
ExprPtr Parser::parseInclusiveOrExpression() {
    ExprPtr lhs = parseExclusiveOrExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "|") {
        lex.next();
        ExprPtr rhs = parseExclusiveOrExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = "|";
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// shift-expression: parse left-associative '<<' and '>>' using assignment-expression as base
// shift-expression: parse left-associative '<<' and '>>' using additive-expression as base
ExprPtr Parser::parseShiftExpression() {
    ExprPtr lhs = parseAdditiveExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
        std::string op = lex.peek()->lexeme();
        if (op != "<<" && op != ">>") break;
        lex.next();
        ExprPtr rhs = parseAdditiveExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = op;
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// multiplicative-expression: '*', '/', '%' (left-associative)
ExprPtr Parser::parseMultiplicativeExpression() {
    ExprPtr lhs = parseCastExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
        std::string op = lex.peek()->lexeme();
        if (op != "*" && op != "/" && op != "%") break;
        lex.next();
        ExprPtr rhs = parseUnaryExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = op;
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// cast-expression: unary-expression | ( type-name ) cast-expression
ExprPtr Parser::parseCastExpression() {
    if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "(") {
        // consume '('
        lex.next();
        // heuristic: if next token starts a type-name, parse declaration specifiers
        bool isTypeNameStart = false;
        if (lex.peek()) {
            auto t = lex.peek();
            if (t->kind() == TokenKind::Keyword) {
                static const std::unordered_set<std::string> types = {
                    "void","char","short","int","long","float","double","signed","unsigned",
                    "_Bool","_Complex","_Imaginary","struct","union","enum",
                    // Type qualifiers can lead a type-name in a cast:
                    //   (const unsigned char *)p
                    "const","volatile","restrict","_Atomic"
                };
                if (types.count(t->lexeme())) isTypeNameStart = true;
            } else if (t->kind() == TokenKind::Identifier) {
                if (typedef_names.count(t->lexeme())) isTypeNameStart = true;
            }
        }

        if (isTypeNameStart) {
            // parse type-name (using declaration specifiers as heuristic)
            auto specs = parseDeclarationSpecifiers();
            // Minimal abstract-declarator support: count leading `*`s so we
            // can recognise pointer casts like `(unsigned char *)p`. We don't
            // attempt array/function abstract declarators yet — they aren't
            // needed by libc's cast usage and only show up in odd corners.
            int castPointerDepth = 0;
            while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator
                   && lex.peek()->lexeme() == "*") {
                lex.next();
                castPointerDepth++;
                // Skip any qualifier tokens (const/volatile/restrict) after `*`.
                while (lex.peek() && lex.peek()->kind() == TokenKind::Keyword) {
                    const auto &kw = lex.peek()->lexeme();
                    if (kw == "const" || kw == "volatile" || kw == "restrict") lex.next();
                    else break;
                }
            }
            if (!(lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")")) {
                if (lex.peek()) {
                    wvmcc::Diagnostic d; d.severity = wvmcc::Diagnostic::Severity::Error; d.message = "expected ')' after type name in cast"; d.span = lex.peek()->span; diagnostics.push_back(std::move(d));
                }
            } else {
                lex.next();
            }
            // If a '{' follows the type-name, this is a compound-literal: (type-name) { initializer-list }
            if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "{") {
                auto clit = make_ast<CompoundLiteral>();
                // build a minimal TypeNode from specs
                auto tn = make_ast<TypeNode>();
                if (!specs.typeSpecifiers.empty()) {
                    auto &ts = specs.typeSpecifiers.front();
                    if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) { tn->kind = TypeNode::Kind::Builtin; tn->simple = ts.simple; }
                    else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion && ts.su) { tn->kind = (ts.su->kind == StructOrUnionSpecifier::Kind::Struct) ? TypeNode::Kind::Struct : TypeNode::Kind::Union; tn->su = ts.su; }
                    else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Other) { tn->kind = TypeNode::Kind::Builtin; tn->text = ts.text; }
                    else { tn->kind = TypeNode::Kind::Builtin; tn->text = "type"; }
                } else { tn->kind = TypeNode::Kind::Builtin; tn->text = "type"; }
                clit->type = tn;
                // parse initializer-list using existing helper
                clit->init = parseInitializer();
                clit->kind = Expr::Kind::CompoundLiteral;
                clit->span = clit->init ? clit->init->span : SourceSpan{};
                return clit;
            }

            ExprPtr rhs = parseCastExpression();
            auto ce = make_ast<CastExpr>();
            ce->expr = rhs;
            ce->kind = Expr::Kind::Cast;
            // build a minimal TypeNode from specs
            auto tn = make_ast<TypeNode>();
            if (!specs.typeSpecifiers.empty()) {
                // try to stringify first specifier
                auto &ts = specs.typeSpecifiers.front();
                if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Simple) { tn->kind = TypeNode::Kind::Builtin; tn->simple = ts.simple; }
                else if (ts.kind == DeclarationSpecifiers::TypeSpecifier::Kind::Other) { tn->kind = TypeNode::Kind::Builtin; tn->text = ts.text; }
                else { tn->kind = TypeNode::Kind::Builtin; tn->text = "type"; }
            } else { tn->kind = TypeNode::Kind::Builtin; tn->text = "type"; }
            // Wrap in Pointer layers for each `*` in the abstract declarator.
            for (int i = 0; i < castPointerDepth; ++i) {
                auto wrap = make_ast<TypeNode>();
                wrap->kind = TypeNode::Kind::Pointer;
                wrap->pointee = tn;
                tn = wrap;
            }
            ce->type = tn;
            ce->span = ce->expr ? ce->expr->span : SourceSpan{};
            return ce;
        }

        // not a type-name: treat as parenthesized expression
        ExprPtr inner = parseAssignmentExpression();
        if (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == ")") lex.next();
        return inner;
    }
    // otherwise unary-expression
    return parseUnaryExpression();
}

// additive-expression: '+' and '-' (left-associative) using multiplicative-expression as operand
ExprPtr Parser::parseAdditiveExpression() {
    ExprPtr lhs = parseMultiplicativeExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
        std::string op = lex.peek()->lexeme();
        if (op != "+" && op != "-") break;
        lex.next();
        ExprPtr rhs = parseMultiplicativeExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = op;
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// relational-expression: < > <= >= using shift-expression as operand
ExprPtr Parser::parseRelationalExpression() {
    ExprPtr lhs = parseShiftExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
        std::string op = lex.peek()->lexeme();
        if (op != "<" && op != ">" && op != "<=" && op != ">=") break;
        lex.next();
        ExprPtr rhs = parseShiftExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = op;
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// equality-expression: == != using relational-expression as operand
ExprPtr Parser::parseEqualityExpression() {
    ExprPtr lhs = parseRelationalExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator) {
        std::string op = lex.peek()->lexeme();
        if (op != "==" && op != "!=") break;
        lex.next();
        ExprPtr rhs = parseRelationalExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = op;
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
}

// AND-expression: bitwise '&' using equality-expression as operand
ExprPtr Parser::parseAndExpression() {
    ExprPtr lhs = parseEqualityExpression();
    if (!lhs) return nullptr;
    while (lex.peek() && lex.peek()->kind() == TokenKind::Punctuator && lex.peek()->lexeme() == "&") {
        lex.next();
        ExprPtr rhs = parseEqualityExpression();
        auto be = make_ast<BinaryExpr>();
        be->op = "&";
        be->lhs = lhs;
        be->rhs = rhs;
        be->kind = Expr::Kind::Binary;
        be->span = lhs->span;
        if (rhs) be->span.end = rhs->span.end;
        lhs = be;
    }
    return lhs;
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
    init->expr = parseAssignmentExpression();
    return init;
}

Designator Parser::parseDesignator() {
    Designator d;
    if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="[") {
        // array-index designator
        lex.next();
        auto e = parseConditionalExpression();
        d.kind = Designator::Kind::Index;
        d.index = e;
        if (lex.peek() && lex.peek()->kind()==TokenKind::Punctuator && lex.peek()->lexeme()=="]") lex.next();
            // designator index expression recorded; semantic checks (constantness) moved to Semantic
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

