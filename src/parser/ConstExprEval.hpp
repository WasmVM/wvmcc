// Small integer constant-expression evaluator used by the parser diagnostics
#pragma once

#include "AST.hpp"
#include <optional>
#include <functional>

namespace wvmcc::parser {

class ConstExprEvaluator {
public:
    // Evaluate expression to integer value; returns std::nullopt if not an integer constant.
    static std::optional<long long> evalIntegerConstantExpr(const ExprPtr &e);
    // Test whether expression is an integer constant expression.
    static bool isIntegerConstantExpr(const ExprPtr &e) { return evalIntegerConstantExpr(e).has_value(); }

    // #81: A type resolver lets `sizeof`/`_Alignof` of a *declared object* (or
    // member / array element) evaluate, which the standalone parser-time
    // evaluator cannot do — it has no symbol table. The semantic pass supplies
    // one (backed by typeOfExpr) for the duration of a call. Returns the TypeNode
    // of an expression, or null if unknown. RAII set/clear via ResolverScope.
    using TypeResolver = std::function<TypeNodePtr(const ExprPtr &)>;
    struct ResolverScope {
        explicit ResolverScope(TypeResolver r);
        ~ResolverScope();
        ResolverScope(const ResolverScope &) = delete;
        ResolverScope &operator=(const ResolverScope &) = delete;
    };

    // #81: True if `e` contains a `sizeof`/`_Alignof` applied to an *expression*
    // operand (not a type-name and not a string literal) — i.e. a form whose
    // value the parser-time evaluator cannot determine without a symbol table,
    // so the static-assert check should defer it to semantic analysis rather
    // than reject it as a non-constant expression.
    static bool dependsOnUnresolvedSizeof(const ExprPtr &e);
};

} // namespace wvmcc::parser
