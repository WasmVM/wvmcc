#include "ConstExprEval.hpp"
#include "AST.hpp"

namespace wvmcc::parser {

std::optional<long long> ConstExprEvaluator::evalIntegerConstantExpr(const ExprPtr &e) {
    if (!e) return std::nullopt;
    if (e->kind == Expr::Kind::Integer) {
        auto il = std::static_pointer_cast<IntegerLiteral>(e);
        return il->value;
    }
    if (e->kind == Expr::Kind::Unary) {
        auto ue = std::static_pointer_cast<UnaryExpr>(e);
        if (!ue->rhs) return std::nullopt;
        // increment/decrement and function-call are not allowed in constant expressions
        if (ue->op == "++" || ue->op == "--") return std::nullopt;
        auto v = evalIntegerConstantExpr(ue->rhs);
        if (!v.has_value()) return std::nullopt;
        if (ue->op == "-") return -(*v);
        if (ue->op == "+") return *v;
        if (ue->op == "~") return ~(*v);
        return std::nullopt;
    }
    if (e->kind == Expr::Kind::PostfixUnary) {
        // postfix ++/-- are not allowed in constant expressions
        return std::nullopt;
    }
    if (e->kind == Expr::Kind::Call) {
        // function calls are not constant expressions
        return std::nullopt;
    }
    if (e->kind == Expr::Kind::Binary) {
        auto be = std::static_pointer_cast<BinaryExpr>(e);
        if (!be->lhs || !be->rhs) return std::nullopt;
        // comma operator is not allowed in constant expressions
        if (be->op == ",") return std::nullopt;
        // assignment operators are not constant expressions
        static const std::vector<std::string> assignOps = {"=","*=","/=","%=","+=","-=","<<=",">>=","&=","^=","|="};
        for (const auto &aop : assignOps) if (be->op == aop) return std::nullopt;
        auto L = evalIntegerConstantExpr(be->lhs);
        auto R = evalIntegerConstantExpr(be->rhs);
        if (!L.has_value() || !R.has_value()) return std::nullopt;
        const long long l = *L;
        const long long r = *R;
        if (be->op == "+") return l + r;
        if (be->op == "-") return l - r;
        if (be->op == "*") return l * r;
        if (be->op == "/") { if (r == 0) return std::nullopt; return l / r; }
        if (be->op == "%") { if (r == 0) return std::nullopt; return l % r; }
        if (be->op == "<<") return l << r;
        if (be->op == ">>") return l >> r;
        if (be->op == "&") return l & r;
        if (be->op == "|") return l | r;
        if (be->op == "^") return l ^ r;
    }
    if (e->kind == Expr::Kind::Ternary) {
        auto te = std::static_pointer_cast<TernaryExpr>(e);
        if (!te->cond) return std::nullopt;
        auto c = evalIntegerConstantExpr(te->cond);
        if (!c.has_value()) return std::nullopt;
        if (*c) return evalIntegerConstantExpr(te->thenExpr);
        return evalIntegerConstantExpr(te->elseExpr);
    }
    return std::nullopt;
}

} // namespace wvmcc::parser
