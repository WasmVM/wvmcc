// Small integer constant-expression evaluator used by the parser diagnostics
#pragma once

#include "AST.hpp"
#include <optional>

namespace wvmcc::parser {

class ConstExprEvaluator {
public:
    // Evaluate expression to integer value; returns std::nullopt if not an integer constant.
    static std::optional<long long> evalIntegerConstantExpr(const ExprPtr &e);
    // Test whether expression is an integer constant expression.
    static bool isIntegerConstantExpr(const ExprPtr &e) { return evalIntegerConstantExpr(e).has_value(); }
};

} // namespace wvmcc::parser
