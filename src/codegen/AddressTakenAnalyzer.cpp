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

void AddressTakenAnalyzer::walk(const wvmcc::parser::ExprPtr& expr, std::unordered_set<std::string>& addressTakenNames) {
    if (!expr) {
        return;
    }
    
    // Handle unary expressions (specifically address-of operator)
    if (expr->kind == wvmcc::parser::Expr::Kind::Unary) {
        const auto& unaryExpr = static_cast<const wvmcc::parser::UnaryExpr&>(*expr);
        if (unaryExpr.op == "&") {
            // This is an address-of operation - find the identifier being addressed
            if (unaryExpr.operand->kind == wvmcc::parser::Expr::Kind::Identifier) {
                const auto& identExpr = static_cast<const wvmcc::parser::IdentifierExpr&>(*unaryExpr.operand);
                addressTakenNames.insert(identExpr.name);
            }
            // For more complex expressions like &(*ptr), we need to recursively walk
            walk(unaryExpr.operand, addressTakenNames);
        } else {
            // For other unary operations, recursively walk the operand
            walk(unaryExpr.operand, addressTakenNames);
        }
    }
    // Handle binary expressions (for cases like a = &var)
    else if (expr->kind == wvmcc::parser::Expr::Kind::Binary) {
        const auto& binaryExpr = static_cast<const wvmcc::parser::BinaryExpr&>(*expr);
        walk(binaryExpr.left, addressTakenNames);
        walk(binaryExpr.right, addressTakenNames);
    }
    // Handle assignment expressions (like var = &someVar)
    else if (expr->kind == wvmcc::parser::Expr::Kind::Assignment) {
        const auto& assignExpr = static_cast<const wvmcc::parser::AssignmentExpr&>(*expr);
        walk(assignExpr.left, addressTakenNames);
        walk(assignExpr.right, addressTakenNames);
    }
    // Handle identifier expressions (for tracking variables that are used)
    else if (expr->kind == wvmcc::parser::Expr::Kind::Identifier) {
        // We don't add identifiers to addressTakenNames here, as they're just used,
        // not necessarily address-taken. The & operator is what matters.
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
            walk(unaryExpr.operand, addressTakenNames);
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
    // Handle compound literal expressions
    else if (expr->kind == wvmcc::parser::Expr::Kind::CompoundLiteral) {
        const auto& compoundLit = static_cast<const wvmcc::parser::CompoundLiteralExpr&>(*expr);
        // Compound literals don't typically involve address-taking in the same way
        // but we can walk their initializer if it exists
        if (compoundLit.initializer) {
            walk(compoundLit.initializer, addressTakenNames);
        }
    }
}

void AddressTakenAnalyzer::walk(const wvmcc::parser::StmtPtr& stmt, std::unordered_set<std::string>& addressTakenNames) {
    if (!stmt) {
        return;
    }
    
    // Handle compound statements (blocks)
    if (stmt->kind == wvmcc::parser::Stmt::Kind::Compound) {
        const auto& compoundStmt = static_cast<const wvmcc::parser::CompoundStmt&>(*stmt);
        for (const auto& item : compoundStmt.items) {
            walk(item, addressTakenNames);
        }
    }
    // Handle expression statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::Expr) {
        const auto& exprStmt = static_cast<const wvmcc::parser::ExprStmt&>(*stmt);
        walk(exprStmt.expr, addressTakenNames);
    }
    // Handle return statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::Return) {
        const auto& returnStmt = static_cast<const wvmcc::parser::ReturnStmt&>(*stmt);
        if (returnStmt.expr) {
            walk(returnStmt.expr, addressTakenNames);
        }
    }
    // Handle if statements
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::If) {
        const auto& ifStmt = static_cast<const wvmcc::parser::IfStmt&>(*stmt);
        walk(ifStmt.cond, addressTakenNames);
        walk(ifStmt.then, addressTakenNames);
        if (ifStmt.else_) {
            walk(ifStmt.else_, addressTakenNames);
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
            walk(forStmt.init, addressTakenNames);
        }
        if (forStmt.cond) {
            walk(forStmt.cond, addressTakenNames);
        }
        if (forStmt.iter) {
            walk(forStmt.iter, addressTakenNames);
        }
        walk(forStmt.body, addressTakenNames);
    }
    // Handle declaration statements (this is where we might find variables being declared)
    else if (stmt->kind == wvmcc::parser::Stmt::Kind::Declaration) {
        const auto& declStmt = static_cast<const wvmcc::parser::DeclarationStmt&>(*stmt);
        walk(declStmt.decl, addressTakenNames);
    }
}

void AddressTakenAnalyzer::walk(const wvmcc::parser::BlockItemPtr& item, std::unordered_set<std::string>& addressTakenNames) {
    if (!item) {
        return;
    }
    
    // Handle declarations in block items
    if (item->kind == wvmcc::parser::BlockItem::Kind::Declaration) {
        const auto& decl = static_cast<const wvmcc::parser::DeclarationPtr&>(*item);
        walk(decl, addressTakenNames);
    }
    // Handle statements in block items
    else if (item->kind == wvmcc::parser::BlockItem::Kind::Statement) {
        const auto& stmt = static_cast<const wvmcc::parser::StmtPtr&>(*item);
        walk(stmt, addressTakenNames);
    }
}

void AddressTakenAnalyzer::walk(const wvmcc::parser::DeclarationPtr& decl, std::unordered_set<std::string>& addressTakenNames) {
    if (!decl) {
        return;
    }
    
    // For now, we don't need to do anything special with declarations in this context
    // The address-taking analysis is focused on expressions where & operator is used
}

} // namespace wvmcc::codegen