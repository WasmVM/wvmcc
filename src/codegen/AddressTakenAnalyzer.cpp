#include "AddressTakenAnalyzer.hpp"
#include <cassert>

namespace wvmcc::codegen {

std::unordered_set<std::string> AddressTakenAnalyzer::analyze(const wvmcc::parser::FunctionDefPtr& funcDef) {
    std::unordered_set<std::string> addressTakenNames;
    
    if (!funcDef) {
        return addressTakenNames;
    }
    
    // Walk the function body to find address-taken variables
    walk(funcDef->body, addressTakenNames);
    
    return addressTakenNames;
}

void AddressTakenAnalyzer::walk(const std::vector<wvmcc::parser::BlockItemPtr>& blockItems, std::unordered_set<std::string>& addressTakenNames) {
    for (const auto& item : blockItems) {
        walk(item, addressTakenNames);
    }
}

void AddressTakenAnalyzer::walk(const wvmcc::parser::BlockItemPtr& item, std::unordered_set<std::string>& addressTakenNames) {
    if (!item) {
        return;
    }
    
    // Handle declarations in block items
    std::visit([&](const auto& variantItem) {
        using T = std::decay_t<decltype(variantItem)>;
        if constexpr (std::is_same_v<T, wvmcc::parser::DeclarationPtr>) {
            // Walk the initializer so that &var inside it is detected. Both
            // kinds: `int *p = &v;` is an Expr initializer, but &v appears in
            // List initializers just as legally -- `struct S s = { &v, 7 };`,
            // `int *a[2] = { &v };` -- and skipping those left v out of the
            // frame, so the emitted address-of failed module validation.
            if (variantItem && variantItem->initializer) {
                walkInitializer(*variantItem->initializer, addressTakenNames);
            }
        } else if constexpr (std::is_same_v<T, wvmcc::parser::StmtPtr>) {
            walk(variantItem, addressTakenNames);
        } else {
            // Static assert - ignore for now
        }
    }, item->item);
}

void AddressTakenAnalyzer::walk(const wvmcc::parser::StmtPtr& stmt, std::unordered_set<std::string>& addressTakenNames) {
    if (!stmt) {
        return;
    }
    
    // Handle compound statements (blocks)
    if (stmt->kind == wvmcc::parser::Stmt::Kind::Compound) {
        const auto& compoundStmt = static_cast<const wvmcc::parser::CompoundStmt&>(*stmt);
        walk(compoundStmt.items, addressTakenNames);
    }
    // Handle expression statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::Expr) {
        const auto& exprStmt = static_cast<const wvmcc::parser::ExprStmt&>(*stmt);
        walk(exprStmt.expr, addressTakenNames);
    }
    // Handle return statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::Return) {
        const auto& returnStmt = static_cast<const wvmcc::parser::ReturnStmt&>(*stmt);
        if (returnStmt.value) {
            walk(returnStmt.value.value(), addressTakenNames);
        }
    }
    // Handle if statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::If) {
        const auto& ifStmt = static_cast<const wvmcc::parser::IfStmt&>(*stmt);
        walk(ifStmt.cond, addressTakenNames);
        walk(ifStmt.thenStmt, addressTakenNames);
        if (ifStmt.elseStmt) {
            walk(ifStmt.elseStmt.value(), addressTakenNames);
        }
    }
    // Handle while statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::While) {
        const auto& whileStmt = static_cast<const wvmcc::parser::WhileStmt&>(*stmt);
        walk(whileStmt.cond, addressTakenNames);
        walk(whileStmt.body, addressTakenNames);
    }
    // Handle for statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::For) {
        const auto& forStmt = static_cast<const wvmcc::parser::ForStmt&>(*stmt);
        if (forStmt.init) {
            walk(forStmt.init.value(), addressTakenNames);
        }
        if (forStmt.cond) {
            walk(forStmt.cond.value(), addressTakenNames);
        }
        if (forStmt.step) {
            walk(forStmt.step.value(), addressTakenNames);
        }
        walk(forStmt.body, addressTakenNames);
    }
    // Handle other statement types that don't contain expressions we care about
    else {
        // For now, we don't need to handle other statement types for address-taking analysis
    }
}

void AddressTakenAnalyzer::walk(const wvmcc::parser::ExprPtr& expr, std::unordered_set<std::string>& addressTakenNames) {
    if (!expr) {
        return;
    }
    
    // Handle unary expressions (specifically address-of operator)
    if (expr->kind == wvmcc::parser::Expr::Kind::Unary) {
        const auto& unaryExpr = static_cast<const wvmcc::parser::UnaryExpr&>(*expr);
        if (unaryExpr.op == "&") {
            // This is an address-of operation - find the identifier being addressed
            if (unaryExpr.rhs->kind == wvmcc::parser::Expr::Kind::Ident) {
                const auto& identExpr = static_cast<const wvmcc::parser::IdentifierExpr&>(*unaryExpr.rhs);
                addressTakenNames.insert(identExpr.name);
            }
            // For more complex expressions like &(*ptr), we need to recursively walk
            walk(unaryExpr.rhs, addressTakenNames);
        } else {
            // For other unary operations, recursively walk the operand
            walk(unaryExpr.rhs, addressTakenNames);
        }
    }
    // Handle binary expressions (for cases like a = &var)
    else if (expr->kind == wvmcc::parser::Expr::Kind::Binary) {
        const auto& binaryExpr = static_cast<const wvmcc::parser::BinaryExpr&>(*expr);
        walk(binaryExpr.lhs, addressTakenNames);
        walk(binaryExpr.rhs, addressTakenNames);
    }
    else if (expr->kind == wvmcc::parser::Expr::Kind::Ident) {
        // identifiers that are merely read are not address-taken
    }
    // Handle function call expressions
    else if (expr->kind == wvmcc::parser::Expr::Kind::Call) {
        const auto& callExpr = static_cast<const wvmcc::parser::CallExpr&>(*expr);
        walk(callExpr.callee, addressTakenNames);
        // Walk arguments if any
        for (const auto& arg : callExpr.args) {
            walk(arg, addressTakenNames);
        }
    }
    // Handle member access expressions
    else if (expr->kind == wvmcc::parser::Expr::Kind::Member) {
        const auto& memberExpr = static_cast<const wvmcc::parser::MemberExpr&>(*expr);
        walk(memberExpr.base, addressTakenNames);
    }
    // Handle pointer dereference expressions
    else if (expr->kind == wvmcc::parser::Expr::Kind::Unary) {
        const auto& unaryExpr = static_cast<const wvmcc::parser::UnaryExpr&>(*expr);
        if (unaryExpr.op == "*") {
            walk(unaryExpr.rhs, addressTakenNames);
        }
    }
    // Handle array index expressions
    else if (expr->kind == wvmcc::parser::Expr::Kind::Index) {
        const auto& indexExpr = static_cast<const wvmcc::parser::IndexExpr&>(*expr);
        walk(indexExpr.base, addressTakenNames);
        walk(indexExpr.index, addressTakenNames);
    }
    // Handle cast expressions
    else if (expr->kind == wvmcc::parser::Expr::Kind::Cast) {
        const auto& castExpr = static_cast<const wvmcc::parser::CastExpr&>(*expr);
        walk(castExpr.expr, addressTakenNames);
    }
    else if (expr->kind == wvmcc::parser::Expr::Kind::CompoundLiteral) {
        // The literal's initializer list is evaluated like any other, so
        // `(struct S){ &v }` takes v's address.
        const auto& cl = static_cast<const wvmcc::parser::CompoundLiteral&>(*expr);
        walkInitializer(cl.init, addressTakenNames);
    }
    // Handle conditional expressions: both arms (and the condition) evaluate
    // in address contexts -- `p = c ? &v : &w;` takes both addresses.
    else if (expr->kind == wvmcc::parser::Expr::Kind::Ternary) {
        const auto& ternary = static_cast<const wvmcc::parser::TernaryExpr&>(*expr);
        walk(ternary.cond, addressTakenNames);
        walk(ternary.thenExpr, addressTakenNames);
        walk(ternary.elseExpr, addressTakenNames);
    }
}

void AddressTakenAnalyzer::walkInitializer(const wvmcc::parser::InitializerPtr& init, std::unordered_set<std::string>& addressTakenNames) {
    if (!init) {
        return;
    }
    if (init->kind == wvmcc::parser::Initializer::Kind::Expr) {
        if (init->expr) {
            walk(init->expr, addressTakenNames);
        }
        return;
    }
    // List: visit every clause, recursing into nested lists. Designator index
    // expressions are constant expressions today, but walking them costs
    // nothing and stays correct if that ever loosens.
    for (const auto& clause : init->clauses) {
        for (const auto& designator : clause.designators) {
            if (designator.index) {
                walk(*designator.index, addressTakenNames);
            }
        }
        walkInitializer(clause.init, addressTakenNames);
    }
}

} // namespace wvmcc::codegen