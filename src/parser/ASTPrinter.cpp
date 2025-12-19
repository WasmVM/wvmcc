// Implementation of ASTPrinter: produce a small XML representation
#include "ASTPrinter.hpp"
#include <sstream>
#include <type_traits>

namespace wvmcc::parser {

ASTPrinter::ASTPrinter(std::ostream &os) : os_(os), indent_(0) {}

void ASTPrinter::writeIndent() {
    for (int i = 0; i < indent_; ++i) os_ << "  ";
}

void ASTPrinter::openTag(const std::string &name, const std::string &attrs) {
    writeIndent();
    os_ << "<" << name;
    if (!attrs.empty()) os_ << " " << attrs;
    os_ << ">\n";
    ++indent_;
}

void ASTPrinter::closeTag(const std::string &name) {
    --indent_;
    writeIndent();
    os_ << "</" << name << ">\n";
}

void ASTPrinter::simpleTag(const std::string &name, const std::string &content) {
    writeIndent();
    os_ << "<" << name << ">" << esc(content) << "</" << name << ">\n";
}

static std::string toString(const SourceSpan &s) {
    std::ostringstream os;
    os << "(" << s.begin.line << "," << s.begin.column << ")-(" << s.end.line << "," << s.end.column << ")";
    return os.str();
}

std::string ASTPrinter::esc(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out += c; break;
        }
    }
    return out;
}

void ASTPrinter::print(const TranslationUnitPtr &tu) {
    openTag("TranslationUnit", "span=\"" + esc(toString(tu->span)) + "\"");
    for (const auto &ext : tu->externals) visitExternalDecl(ext);
    closeTag("TranslationUnit");
}

void ASTPrinter::visitExternalDecl(const ExternalDeclPtr &d) {
    openTag("ExternalDecl", "span=\"" + esc(toString(d->span)) + "\"");
    std::visit([this](auto &&arg){ using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, FunctionDefPtr>) this->visitFunctionDef(arg);
        else if constexpr (std::is_same_v<T, DeclarationPtr>) this->visitDeclaration(arg);
    }, d->decl);
    closeTag("ExternalDecl");
}

void ASTPrinter::visitFunctionDef(const FunctionDefPtr &f) {
    openTag("FunctionDef", "span=\"" + esc(toString(f->span)) + "\"");
    // specifiers
    if (!f->specifiers.empty()) {
        openTag("Specifiers");
        emitSpecifiersEntries(f->specifiers);
        closeTag("Specifiers");
    }
    visitDeclarator(f->declarator);
    // params
    openTag("Parameters");
    for (const auto &p : f->params) {
        openTag("Param");
        if (p.typeSpec) simpleTag("TypeSpec", *p.typeSpec);
        if (p.declarator) visitDeclarator(p.declarator);
        if (p.defaultValue) visitExpr(*p.defaultValue);
        closeTag("Param");
    }
    closeTag("Parameters");
    // body
    openTag("Body");
    for (const auto &bi : f->body) visitBlockItem(bi);
    closeTag("Body");
    closeTag("FunctionDef");
}

void ASTPrinter::visitDeclaration(const DeclarationPtr &d) {
    openTag("Declaration", "span=\"" + esc(toString(d->span)) + "\"");
    if (!d->specifiers.empty()) {
        openTag("Specifiers");
        emitSpecifiersEntries(d->specifiers);
        closeTag("Specifiers");
    }
    if (d->declarator) visitDeclarator(d->declarator);
    if (d->initializer) {
        openTag("Initializer");
        visitExpr(*d->initializer);
        closeTag("Initializer");
    }
    closeTag("Declaration");
}

void ASTPrinter::emitSpecifiersEntries(const DeclarationSpecifiers &specs) {
    if (specs.hasStorage(StorageClass::Typedef)) simpleTag("Spec", "typedef");
    if (specs.hasStorage(StorageClass::Extern)) simpleTag("Spec", "extern");
    if (specs.hasStorage(StorageClass::Static)) simpleTag("Spec", "static");
    if (specs.hasStorage(StorageClass::Auto)) simpleTag("Spec", "auto");
    if (specs.hasStorage(StorageClass::Register)) simpleTag("Spec", "register");
    if (specs.hasStorage(StorageClass::ThreadLocal)) simpleTag("Spec", "_Thread_local");

    for (auto &ts : specs.typeSpecifiers) {
        switch (ts.kind) {
            case DeclarationSpecifiers::TypeSpecifier::Kind::Simple: {
                for (auto &tok : ts.simple) {
                    switch (tok) {
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Void: simpleTag("Spec", "void"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Char: simpleTag("Spec", "char"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Short: simpleTag("Spec", "short"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Int: simpleTag("Spec", "int"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Long: simpleTag("Spec", "long"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Float: simpleTag("Spec", "float"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Double: simpleTag("Spec", "double"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Signed: simpleTag("Spec", "signed"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Unsigned: simpleTag("Spec", "unsigned"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Bool: simpleTag("Spec", "_Bool"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Complex: simpleTag("Spec", "_Complex"); break;
                        case DeclarationSpecifiers::SimpleTypeSpecifier::Imaginary: simpleTag("Spec", "_Imaginary"); break;
                        default: break;
                    }
                }
                break;
            }
            case DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion:
                if (ts.su) {
                    std::string kind = (ts.su->kind == StructOrUnionSpecifier::Kind::Struct) ? "struct" : "union";
                    std::string name;
                    if (ts.su->name.has_value()) name = *ts.su->name;
                    if (!ts.su->hasBody) {
                        std::string repr = kind;
                        if (!name.empty()) { repr += " "; repr += name; }
                        simpleTag("Spec", repr);
                    } else {
                        // print detailed struct/union with members
                        openTag(kind + "", (!name.empty() ? std::string("name=\"") + esc(name) + std::string("\"") : std::string()));
                        openTag("Members");
                        for (auto &m : ts.su->members) {
                            openTag("Member");
                            if (!m.specifiers.empty()) {
                                openTag("MemberSpecifiers");
                                emitSpecifiersEntries(m.specifiers);
                                closeTag("MemberSpecifiers");
                            }
                            openTag("Declarators");
                            for (auto &sd : m.declarators) {
                                openTag("Declarator");
                                if (sd.declarator) {
                                    if (!sd.declarator->id.name.empty()) simpleTag("Id", sd.declarator->id.name);
                                    if (sd.declarator->inner) visitDeclarator(*sd.declarator->inner);
                                }
                                if (sd.bitfieldWidth) {
                                    openTag("Bitfield");
                                    visitExpr(*sd.bitfieldWidth);
                                    closeTag("Bitfield");
                                }
                                closeTag("Declarator");
                            }
                            closeTag("Declarators");
                            closeTag("Member");
                        }
                        closeTag("Members");
                        closeTag(kind + "");
                    }
                } else if (!ts.text.empty()) simpleTag("Spec", ts.text);
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::Enum:
                if (ts.en) {
                    std::string name;
                    if (ts.en->name.has_value()) name = *ts.en->name;
                    if (!ts.en->hasBody) {
                        std::string repr = "enum";
                        if (!name.empty()) { repr += " "; repr += name; }
                        simpleTag("Spec", repr);
                    } else {
                        openTag("enum", (!name.empty() ? std::string("name=\"") + esc(name) + std::string("\"") : std::string()));
                        openTag("Enumerators");
                        for (auto &ev : ts.en->enumerators) {
                            openTag("Enumerator");
                            simpleTag("Name", ev.name);
                            if (ev.value) { openTag("Value"); visitExpr(*ev.value); closeTag("Value"); }
                            closeTag("Enumerator");
                        }
                        closeTag("Enumerators");
                        closeTag("enum");
                    }
                } else if (!ts.text.empty()) simpleTag("Spec", ts.text);
                break;
            case DeclarationSpecifiers::TypeSpecifier::Kind::TypedefName:
            case DeclarationSpecifiers::TypeSpecifier::Kind::Other:
                if (!ts.text.empty()) simpleTag("Spec", ts.text);
                break;
        }
    }

    if (specs.hasTypeQual(TypeQualifier::Const)) simpleTag("Spec", "const");
    if (specs.hasTypeQual(TypeQualifier::Volatile)) simpleTag("Spec", "volatile");
    if (specs.hasTypeQual(TypeQualifier::Restrict)) simpleTag("Spec", "restrict");
    if (specs.hasTypeQual(TypeQualifier::Atomic)) simpleTag("Spec", "_Atomic");
    if (specs.hasFuncSpec(FunctionSpecifier::Inline)) simpleTag("Spec", "inline");
    if (specs.hasFuncSpec(FunctionSpecifier::NoReturn)) simpleTag("Spec", "_Noreturn");
    for (auto &a : specs.alignSpec) simpleTag("Spec", a);
}

void ASTPrinter::visitDeclarator(const DeclaratorPtr &d) {
    openTag("Declarator", "span=\"" + esc(toString(d->span)) + "\"");
    simpleTag("Id", d->id.name);
    if (d->inner) {
        openTag("Inner");
        visitDeclarator(*d->inner);
        closeTag("Inner");
    }
    closeTag("Declarator");
}

void ASTPrinter::visitTypeNode(const TypeNodePtr &t) {
    openTag("TypeNode", "kind=\"" + std::to_string(static_cast<int>(t->kind)) + "\"");
    simpleTag("Repr", t->repr);
    closeTag("TypeNode");
}

void ASTPrinter::visitBlockItem(const BlockItemPtr &b) {
    openTag("BlockItem", "span=\"" + esc(toString(b->span)) + "\"");
    if (std::holds_alternative<DeclarationPtr>(b->item)) {
        visitDeclaration(std::get<DeclarationPtr>(b->item));
    } else {
        visitStmt(std::get<StmtPtr>(b->item));
    }
    closeTag("BlockItem");
}

void ASTPrinter::visitStmt(const StmtPtr &s) {
    if (!s) return;
    openTag("Stmt", "kind=\"" + std::to_string(static_cast<int>(s->kind)) + "\" span=\"" + esc(toString(s->span)) + "\"");
    switch (s->kind) {
        case Stmt::Kind::Expr: {
            auto es = std::static_pointer_cast<ExprStmt>(s);
            if (es->expr) visitExpr(es->expr);
            break;
        }
        case Stmt::Kind::Compound: {
            auto cs = std::static_pointer_cast<CompoundStmt>(s);
            for (auto &it : cs->items) visitBlockItem(it);
            break;
        }
        case Stmt::Kind::If: {
            auto is = std::static_pointer_cast<IfStmt>(s);
            openTag("If");
            if (is->cond) visitExpr(is->cond);
            if (is->thenStmt) visitStmt(is->thenStmt);
            if (is->elseStmt) visitStmt(*is->elseStmt);
            closeTag("If");
            break;
        }
        case Stmt::Kind::While: {
            auto ws = std::static_pointer_cast<WhileStmt>(s);
            openTag("While");
            if (ws->cond) visitExpr(ws->cond);
            if (ws->body) visitStmt(ws->body);
            closeTag("While");
            break;
        }
        case Stmt::Kind::For: {
            auto fs = std::static_pointer_cast<ForStmt>(s);
            openTag("For");
            if (fs->init) visitBlockItem(*fs->init);
            if (fs->cond) visitExpr(*fs->cond);
            if (fs->step) visitExpr(*fs->step);
            if (fs->body) visitStmt(fs->body);
            closeTag("For");
            break;
        }
        case Stmt::Kind::Return: {
            auto rs = std::static_pointer_cast<ReturnStmt>(s);
            if (rs->value) visitExpr(*rs->value);
            break;
        }
        default:
            // other kinds: emit empty marker
            break;
    }
    closeTag("Stmt");
}

void ASTPrinter::visitExpr(const ExprPtr &e) {
    if (!e) return;
    openTag("Expr", "kind=\"" + std::to_string(static_cast<int>(e->kind)) + "\" span=\"" + esc(toString(e->span)) + "\"");
    switch (e->kind) {
        case Expr::Kind::Ident: {
            auto id = std::static_pointer_cast<IdentifierExpr>(e);
            simpleTag("Identifier", id->name);
            break;
        }
        case Expr::Kind::Integer: {
            auto il = std::static_pointer_cast<IntegerLiteral>(e);
            simpleTag("Integer", il->raw.empty() ? std::to_string(il->value) : il->raw);
            break;
        }
        case Expr::Kind::String: {
            auto sl = std::static_pointer_cast<StringLiteral>(e);
            simpleTag("String", sl->value);
            break;
        }
        case Expr::Kind::Unary: {
            auto ue = std::static_pointer_cast<UnaryExpr>(e);
            simpleTag("Op", ue->op);
            if (ue->rhs) visitExpr(ue->rhs);
            break;
        }
        case Expr::Kind::Binary: {
            auto be = std::static_pointer_cast<BinaryExpr>(e);
            simpleTag("Op", be->op);
            if (be->lhs) visitExpr(be->lhs);
            if (be->rhs) visitExpr(be->rhs);
            break;
        }
        case Expr::Kind::Ternary: {
            auto te = std::static_pointer_cast<TernaryExpr>(e);
            if (te->cond) visitExpr(te->cond);
            if (te->thenExpr) visitExpr(te->thenExpr);
            if (te->elseExpr) visitExpr(te->elseExpr);
            break;
        }
        case Expr::Kind::Call: {
            auto ce = std::static_pointer_cast<CallExpr>(e);
            if (ce->callee) visitExpr(ce->callee);
            openTag("Args");
            for (auto &a : ce->args) visitExpr(a);
            closeTag("Args");
            break;
        }
        case Expr::Kind::Member: {
            auto me = std::static_pointer_cast<MemberExpr>(e);
            if (me->base) visitExpr(me->base);
            simpleTag("Member", me->member + (me->isArrow ? "(->)" : "."));
            break;
        }
        case Expr::Kind::Index: {
            auto ie = std::static_pointer_cast<IndexExpr>(e);
            if (ie->base) visitExpr(ie->base);
            if (ie->index) visitExpr(ie->index);
            break;
        }
        default:
            break;
    }
    closeTag("Expr");
}

} // namespace wvmcc::parser
