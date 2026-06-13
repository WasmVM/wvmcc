#include "ASTVisitor.hpp"
#include <memory>
#include <variant>

namespace wvmcc::parser {

void ASTVisitor::traverseTranslationUnit(const TranslationUnitPtr &tu) {
    if (!tu) return;
    for (auto &ext : tu->externals) traverseExternalDecl(ext);
}

void ASTVisitor::traverseExternalDecl(const ExternalDeclPtr &ext) {
    if (!ext) return;
    if (std::holds_alternative<FunctionDefPtr>(ext->decl)) {
        auto f = std::get<FunctionDefPtr>(ext->decl);
        onFunctionDef(f);
        traverseFunction(f);
    } else if (std::holds_alternative<DeclarationPtr>(ext->decl)) {
        auto d = std::get<DeclarationPtr>(ext->decl);
        onDeclaration(d);
        traverseDeclaration(d);
    } else if (std::holds_alternative<ExternalDecl::StaticAssertPtr>(ext->decl)) {
        auto sa = std::get<ExternalDecl::StaticAssertPtr>(ext->decl);
        onStaticAssert(sa);
        if (sa && sa->expr) traverseExpr(sa->expr);
        // message is a string literal; traverse it for completeness
        if (sa && sa->message) traverseExpr(sa->message);
    }
}

// Report a *full* expression to onExpr (once, at the top of the tree) and then
// recurse for the per-node hooks (onIdent, etc.).
void ASTVisitor::traverseFullExpr(const ExprPtr &e) {
    if (!e) return;
    onExpr(e);
    traverseExpr(e);
}

void ASTVisitor::traverseFunction(const FunctionDefPtr &f) {
    if (!f) return;
    onEnterFunction(f);
    // params
    for (const auto &p : f->params) {
        if (p.defaultValue) traverseExpr(p.defaultValue.value());
    }
    // body — the function body is itself a block scope
    onEnterBlock();
    for (const auto &bi : f->body) {
        if (std::holds_alternative<DeclarationPtr>(bi->item)) {
            auto d = std::get<DeclarationPtr>(bi->item);
            onDeclaration(d);
            traverseDeclaration(d);
        } else if (std::holds_alternative<ExternalDecl::StaticAssertPtr>(bi->item)) {
            auto sa = std::get<ExternalDecl::StaticAssertPtr>(bi->item);
            onStaticAssert(sa);
            if (sa && sa->expr) traverseExpr(sa->expr);
            if (sa && sa->message) traverseExpr(sa->message);
        } else {
            traverseStmt(std::get<StmtPtr>(bi->item));
        }
    }
    onExitBlock();
    onExitFunction(f);
}

void ASTVisitor::traverseDeclaration(const DeclarationPtr &d) {
    if (!d) return;
    if (d->initializer) traverseInit(d->initializer.value());
}

void ASTVisitor::traverseInit(const InitializerPtr &in) {
    if (!in) return;
    if (in->kind == Initializer::Kind::Expr) traverseFullExpr(in->expr);
    else {
        for (const auto &c : in->clauses) {
            for (const auto &des : c.designators) {
                if (des.kind == Designator::Kind::Index && des.index) traverseExpr(des.index.value());
            }
            traverseInit(c.init);
        }
    }
}

void ASTVisitor::traverseStmt(const StmtPtr &s) {
    if (!s) return;
    switch (s->kind) {
        case Stmt::Kind::Expr: {
            auto es = std::dynamic_pointer_cast<ExprStmt>(s);
            if (es && es->expr) traverseFullExpr(es->expr);
            break;
        }
        case Stmt::Kind::Compound: {
            auto cs = std::dynamic_pointer_cast<CompoundStmt>(s);
            if (cs) {
                onEnterBlock();
                for (const auto &bi : cs->items) {
                    if (std::holds_alternative<DeclarationPtr>(bi->item)) {
                        auto d = std::get<DeclarationPtr>(bi->item);
                        onDeclaration(d);
                        traverseDeclaration(d);
                    } else if (std::holds_alternative<ExternalDecl::StaticAssertPtr>(bi->item)) {
                        auto sa = std::get<ExternalDecl::StaticAssertPtr>(bi->item);
                        onStaticAssert(sa);
                        if (sa && sa->expr) traverseExpr(sa->expr);
                        if (sa && sa->message) traverseExpr(sa->message);
                    } else traverseStmt(std::get<StmtPtr>(bi->item));
                }
                onExitBlock();
            }
            break;
        }
        case Stmt::Kind::If: {
            auto ifs = std::dynamic_pointer_cast<IfStmt>(s);
            if (ifs) {
                traverseFullExpr(ifs->cond);
                traverseStmt(ifs->thenStmt);
                if (ifs->elseStmt) traverseStmt(ifs->elseStmt.value());
            }
            break;
        }
        case Stmt::Kind::While: {
            auto ws = std::dynamic_pointer_cast<WhileStmt>(s);
            if (ws) {
                traverseFullExpr(ws->cond);
                traverseStmt(ws->body);
            }
            break;
        }
        case Stmt::Kind::For: {
            auto fs = std::dynamic_pointer_cast<ForStmt>(s);
            if (fs) {
                // A `for` statement introduces its own block scope: a
                // declaration in the init-clause is local to the loop (C
                // 6.8.5p5), so it must not collide with prior sibling loops.
                onEnterBlock();
                if (fs->init) {
                    auto bi = fs->init.value();
                    if (std::holds_alternative<DeclarationPtr>(bi->item)) {
                        auto d = std::get<DeclarationPtr>(bi->item);
                        onDeclaration(d);
                        traverseDeclaration(d);
                    } else traverseStmt(std::get<StmtPtr>(bi->item));
                }
                if (fs->cond) traverseFullExpr(fs->cond.value());
                if (fs->step) traverseFullExpr(fs->step.value());
                traverseStmt(fs->body);
                onExitBlock();
            }
            break;
        }
        case Stmt::Kind::Return: {
            auto rs = std::dynamic_pointer_cast<ReturnStmt>(s);
            if (rs && rs->value) traverseFullExpr(rs->value.value());
            break;
        }
        case Stmt::Kind::DoWhile: {
            auto dws = std::dynamic_pointer_cast<DoWhileStmt>(s);
            if (dws) {
                traverseStmt(dws->body);
                traverseFullExpr(dws->cond);
            }
            break;
        }
        case Stmt::Kind::Switch: {
            auto ss = std::dynamic_pointer_cast<SwitchStmt>(s);
            if (ss) {
                traverseFullExpr(ss->cond);
                traverseStmt(ss->body);
            }
            break;
        }
        case Stmt::Kind::Case: {
            auto cs = std::dynamic_pointer_cast<CaseStmt>(s);
            if (cs) {
                traverseFullExpr(cs->value);
                traverseStmt(cs->stmt);
            }
            break;
        }
        case Stmt::Kind::Default: {
            auto ds = std::dynamic_pointer_cast<DefaultStmt>(s);
            if (ds) traverseStmt(ds->stmt);
            break;
        }
        case Stmt::Kind::Label: {
            auto ls = std::dynamic_pointer_cast<LabelStmt>(s);
            if (ls) traverseStmt(ls->stmt);
            break;
        }
        case Stmt::Kind::Goto:
        case Stmt::Kind::Break:
        case Stmt::Kind::Continue:
        case Stmt::Kind::Empty:
            break;
    }
}

void ASTVisitor::traverseExpr(const ExprPtr &e) {
    if (!e) return;
    switch (e->kind) {
        case Expr::Kind::Ident: onIdent(std::static_pointer_cast<IdentifierExpr>(e)); break;
        case Expr::Kind::Unary: traverseExpr(std::static_pointer_cast<UnaryExpr>(e)->rhs); break;
        case Expr::Kind::PostfixUnary: traverseExpr(std::static_pointer_cast<PostfixUnaryExpr>(e)->base); break;
        case Expr::Kind::Cast: traverseExpr(std::static_pointer_cast<CastExpr>(e)->expr); break;
        case Expr::Kind::Sizeof: {
            auto s = std::static_pointer_cast<SizeofExpr>(e);
            if (s->expr) traverseExpr(s->expr);
            break;
        }
        case Expr::Kind::AlignOf: break;
        case Expr::Kind::CompoundLiteral: traverseInit(std::static_pointer_cast<CompoundLiteral>(e)->init); break;
        case Expr::Kind::GenericSelection: {
            auto g = std::static_pointer_cast<GenericSelectionExpr>(e);
            traverseExpr(g->controlling);
            for (const auto &a : g->assocs) traverseExpr(a.expr);
            break;
        }
        case Expr::Kind::Binary: {
            auto b = std::static_pointer_cast<BinaryExpr>(e);
            traverseExpr(b->lhs);
            traverseExpr(b->rhs);
            break;
        }
        case Expr::Kind::Ternary: {
            auto t = std::static_pointer_cast<TernaryExpr>(e);
            traverseExpr(t->cond);
            traverseExpr(t->thenExpr);
            traverseExpr(t->elseExpr);
            break;
        }
        case Expr::Kind::Call: {
            auto c = std::static_pointer_cast<CallExpr>(e);
            traverseExpr(c->callee);
            for (const auto &a : c->args) traverseExpr(a);
            break;
        }
        case Expr::Kind::Member: traverseExpr(std::static_pointer_cast<MemberExpr>(e)->base); break;
        case Expr::Kind::Index: {
            auto ix = std::static_pointer_cast<IndexExpr>(e);
            traverseExpr(ix->base);
            traverseExpr(ix->index);
            break;
        }
        case Expr::Kind::Integer:
        case Expr::Kind::Float:
        case Expr::Kind::Char:
        case Expr::Kind::String:
            break;
    }
}

} // namespace wvmcc::parser
