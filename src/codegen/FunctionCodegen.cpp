#include "FunctionCodegen.hpp"
#include "AddressTakenAnalyzer.hpp"
#include <stdexcept>
#include <limits>

namespace wvmcc::codegen {

FunctionCodegen::FunctionCodegen(const TypeMap& typeMap, SymbolTable& symbolTable)
    : typeMap_(typeMap), symbolTable_(symbolTable) {}

WasmVM::WasmFunc FunctionCodegen::generate(const wvmcc::parser::FunctionDefPtr& funcDef,
                                             const wvmcc::parser::Semantic& semantic) {
    WasmVM::WasmFunc func;

    AddressTakenAnalyzer analyzer;
    addressTakenNames_ = analyzer.analyze(funcDef);

    // Parameters occupy local indices 0..n-1 and are not in func.locals.
    symbolTable_.pushScope();
    int paramIdx = 0;
    for (const auto& param : funcDef->params) {
        if (param.declarator && !param.declarator->id.name.empty()) {
            ScalarLocal info;
            info.type = nullptr;
            info.isAddressTaken = false;
            info.localIndex = paramIdx;
            symbolTable_.define(param.declarator->id.name, info);
        }
        ++paramIdx;
    }
    localIndexCounter_ = paramIdx; // locals start after params

    for (const auto& item : funcDef->body) {
        emitBlockItem(item);
    }

    symbolTable_.popScope();

    func.locals = localTypes_;
    func.body = instrBuffer_;
    return func;
}

int FunctionCodegen::allocLocal(const wvmcc::parser::TypeNodePtr& type, bool isAddressTaken) {
    int idx = localIndexCounter_++;
    if (type) {
        localTypes_.push_back(typeMap_.toWasmType(type));
    } else {
        localTypes_.push_back(WasmVM::ValueType::i32);
    }
    return idx;
}

void FunctionCodegen::emit(const WasmVM::WasmInstr& instr) {
    instrBuffer_.push_back(instr);
}

void FunctionCodegen::emitExpr(const wvmcc::parser::ExprPtr& expr, bool needLValue) {
    if (!expr) return;

    using K = wvmcc::parser::Expr::Kind;
    switch (expr->kind) {
    case K::Integer:
        emitIntegerLiteral(static_cast<const wvmcc::parser::IntegerLiteral&>(*expr));
        break;
    case K::Char:
        emitCharLiteral(static_cast<const wvmcc::parser::CharLiteral&>(*expr));
        break;
    case K::Ident:
        emitIdentifierExpr(static_cast<const wvmcc::parser::IdentifierExpr&>(*expr));
        break;
    case K::Binary:
        emitBinaryExpr(static_cast<const wvmcc::parser::BinaryExpr&>(*expr));
        break;
    case K::Unary:
        emitUnaryExpr(static_cast<const wvmcc::parser::UnaryExpr&>(*expr));
        break;
    case K::Cast:
        emitCastExpr(static_cast<const wvmcc::parser::CastExpr&>(*expr));
        break;
    case K::String:
        emitStringLiteral(static_cast<const wvmcc::parser::StringLiteral&>(*expr));
        break;
    case K::Member:
        emitMemberAccessExpr(static_cast<const wvmcc::parser::MemberExpr&>(*expr));
        break;
    case K::Index:
        emitArrayIndexExpr(static_cast<const wvmcc::parser::IndexExpr&>(*expr));
        break;
    case K::CompoundLiteral:
        emitCompoundLiteralExpr(static_cast<const wvmcc::parser::CompoundLiteral&>(*expr));
        break;
    default:
        emit(WasmVM::Instr::Unreachable{});
        break;
    }
}

void FunctionCodegen::emitIntegerLiteral(const wvmcc::parser::IntegerLiteral& expr) {
    if (expr.value >= std::numeric_limits<int32_t>::min() && expr.value <= std::numeric_limits<int32_t>::max()) {
        emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)expr.value});
    } else {
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)expr.value});
    }
}

void FunctionCodegen::emitCharLiteral(const wvmcc::parser::CharLiteral& expr) {
    emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)expr.value});
}

void FunctionCodegen::emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr) {
    auto symbolInfo = symbolTable_.lookup(expr.name);
    if (!symbolInfo) {
        emit(WasmVM::Instr::Unreachable{});
        return;
    }

    std::visit([this](const auto& info) {
        using T = std::decay_t<decltype(info)>;
        if constexpr (std::is_same_v<T, ScalarLocal>) {
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)info.localIndex});
        } else if constexpr (std::is_same_v<T, GlobalScalar>) {
            emit(WasmVM::Instr::Global_get{(WasmVM::index_t)info.globalIndex});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    }, *symbolInfo);
}

void FunctionCodegen::emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr) {
    emitExpr(expr.lhs, false);
    emitExpr(expr.rhs, false);

    auto lhsType = getExprType(expr.lhs);

    if (expr.op == "+") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_add{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_add{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "-") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_sub{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_sub{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "*") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_mul{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_mul{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "/") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_div_s{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_div_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "%") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_rem_s{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_rem_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "==") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_eq{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_eq{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "!=") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_ne{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_ne{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "<") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_lt_s{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_lt_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == ">") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_gt_s{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_gt_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "<=") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_le_s{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_le_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == ">=") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_ge_s{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_ge_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "&") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_and{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_and{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "|") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_or{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_or{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "^") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_xor{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_xor{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "<<") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_shl{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_shl{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == ">>") {
        if (lhsType == WasmVM::ValueType::i32) emit(WasmVM::Instr::I32_shr_s{});
        else if (lhsType == WasmVM::ValueType::i64) emit(WasmVM::Instr::I64_shr_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else {
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr) {
    emitExpr(expr.rhs, false);

    auto exprType = getExprType(expr.rhs);

    if (expr.op == "-") {
        // two's complement negation: ~x + 1
        if (exprType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_const{-1});
            emit(WasmVM::Instr::I32_xor{});
            emit(WasmVM::Instr::I32_const{1});
            emit(WasmVM::Instr::I32_add{});
        } else if (exprType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_const{-1});
            emit(WasmVM::Instr::I64_xor{});
            emit(WasmVM::Instr::I64_const{1});
            emit(WasmVM::Instr::I64_add{});
        } else if (exprType == WasmVM::ValueType::f32) {
            emit(WasmVM::Instr::F32_neg{});
        } else if (exprType == WasmVM::ValueType::f64) {
            emit(WasmVM::Instr::F64_neg{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "~") {
        // bitwise NOT: x ^ -1
        if (exprType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_const{-1});
            emit(WasmVM::Instr::I32_xor{});
        } else if (exprType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_const{-1});
            emit(WasmVM::Instr::I64_xor{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "!") {
        if (exprType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_eqz{});
        } else if (exprType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_eqz{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "+") {
        // no-op
    } else {
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitCastExpr(const wvmcc::parser::CastExpr& expr) {
    emitExpr(expr.expr, false);

    auto targetType = typeMap_.toWasmType(expr.type);
    auto sourceType = getExprType(expr.expr);

    if (sourceType == targetType) {
        // no-op
    } else if (sourceType == WasmVM::ValueType::i32 && targetType == WasmVM::ValueType::i64) {
        emit(WasmVM::Instr::I64_extend_i32_s{});
    } else if (sourceType == WasmVM::ValueType::i64 && targetType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I32_wrap_i64{});
    } else if (sourceType == WasmVM::ValueType::f32 && targetType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I32_trunc_f32_s{});
    } else if (sourceType == WasmVM::ValueType::f64 && targetType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I32_trunc_f64_s{});
    } else if (sourceType == WasmVM::ValueType::f32 && targetType == WasmVM::ValueType::i64) {
        emit(WasmVM::Instr::I64_trunc_f32_s{});
    } else if (sourceType == WasmVM::ValueType::f64 && targetType == WasmVM::ValueType::i64) {
        emit(WasmVM::Instr::I64_trunc_f64_s{});
    } else if (sourceType == WasmVM::ValueType::i32 && targetType == WasmVM::ValueType::f32) {
        emit(WasmVM::Instr::F32_convert_i32_s{});
    } else if (sourceType == WasmVM::ValueType::i64 && targetType == WasmVM::ValueType::f32) {
        emit(WasmVM::Instr::F32_convert_i64_s{});
    } else if (sourceType == WasmVM::ValueType::i32 && targetType == WasmVM::ValueType::f64) {
        emit(WasmVM::Instr::F64_convert_i32_s{});
    } else if (sourceType == WasmVM::ValueType::i64 && targetType == WasmVM::ValueType::f64) {
        emit(WasmVM::Instr::F64_convert_i64_s{});
    } else if (sourceType == WasmVM::ValueType::f32 && targetType == WasmVM::ValueType::f64) {
        emit(WasmVM::Instr::F64_promote_f32{});
    } else if (sourceType == WasmVM::ValueType::f64 && targetType == WasmVM::ValueType::f32) {
        emit(WasmVM::Instr::F32_demote_f64{});
    } else {
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitStringLiteral(const wvmcc::parser::StringLiteral& expr) {
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitMemberAccessExpr(const wvmcc::parser::MemberExpr& expr) {
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitArrayIndexExpr(const wvmcc::parser::IndexExpr& expr) {
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitCompoundLiteralExpr(const wvmcc::parser::CompoundLiteral& expr) {
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitStmt(const wvmcc::parser::StmtPtr& stmt) {
    if (!stmt) return;

    using K = wvmcc::parser::Stmt::Kind;
    switch (stmt->kind) {
    case K::Return:
        emitReturnStmt(static_cast<const wvmcc::parser::ReturnStmt&>(*stmt));
        break;
    case K::Expr:
        emitExprStmt(static_cast<const wvmcc::parser::ExprStmt&>(*stmt));
        break;
    case K::Compound:
        emitCompoundStmt(static_cast<const wvmcc::parser::CompoundStmt&>(*stmt));
        break;
    case K::If:
        emitIfStmt(static_cast<const wvmcc::parser::IfStmt&>(*stmt));
        break;
    case K::While:
        emitWhileStmt(static_cast<const wvmcc::parser::WhileStmt&>(*stmt));
        break;
    case K::For:
        emitForStmt(static_cast<const wvmcc::parser::ForStmt&>(*stmt));
        break;
    case K::Empty:
        break;
    default:
        emit(WasmVM::Instr::Unreachable{});
        break;
    }
}

void FunctionCodegen::emitBlockItem(const wvmcc::parser::BlockItemPtr& item) {
    if (!item) return;

    std::visit([this](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, wvmcc::parser::DeclarationPtr>) {
            if (!v || !v->declarator) return;
            const std::string& name = v->declarator->id.name;

            // Build TypeNode from specifiers (pick the first simple type specifier)
            wvmcc::parser::TypeNodePtr typeNode;
            for (const auto& ts : v->specifiers.typeSpecifiers) {
                if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                    && !ts.simple.empty()) {
                    auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    node->kind = wvmcc::parser::TypeNode::Kind::Builtin;
                    node->simple = ts.simple;
                    typeNode = node;
                    break;
                }
            }

            bool isAddrTaken = addressTakenNames_.count(name) > 0;
            int localIdx = allocLocal(typeNode, isAddrTaken);

            ScalarLocal info;
            info.type = typeNode;
            info.isAddressTaken = isAddrTaken;
            info.localIndex = localIdx;
            symbolTable_.define(name, info);

            // Emit initializer if it is a simple expression
            if (v->initializer
                && (*v->initializer)->kind == wvmcc::parser::Initializer::Kind::Expr
                && (*v->initializer)->expr) {
                emitExpr((*v->initializer)->expr);
                emit(WasmVM::Instr::Local_set{(WasmVM::index_t)localIdx});
            }
        } else if constexpr (std::is_same_v<T, wvmcc::parser::StmtPtr>) {
            emitStmt(v);
        }
        // StaticAssert: ignored in codegen
    }, item->item);
}

void FunctionCodegen::emitReturnStmt(const wvmcc::parser::ReturnStmt& stmt) {
    if (stmt.value) {
        emitExpr(*stmt.value);
    }
    emit(WasmVM::Instr::Return{});
}

void FunctionCodegen::emitExprStmt(const wvmcc::parser::ExprStmt& stmt) {
    if (stmt.expr) {
        emitExpr(stmt.expr);
        emit(WasmVM::Instr::Drop{});
    }
}

void FunctionCodegen::emitCompoundStmt(const wvmcc::parser::CompoundStmt& stmt) {
    symbolTable_.pushScope();
    for (const auto& item : stmt.items) {
        emitBlockItem(item);
    }
    symbolTable_.popScope();
}

void FunctionCodegen::emitIfStmt(const wvmcc::parser::IfStmt& stmt) {
    emitExpr(stmt.cond);
    emit(WasmVM::Instr::If{std::nullopt});
    emitStmt(stmt.thenStmt);
    if (stmt.elseStmt) {
        emit(WasmVM::Instr::Else{});
        emitStmt(*stmt.elseStmt);
    }
    emit(WasmVM::Instr::End{});
}

void FunctionCodegen::emitWhileStmt(const wvmcc::parser::WhileStmt& stmt) {
    // block $break
    //   loop $continue
    //     <cond>; i32.eqz; br_if 1  (exit block)
    //     <body>
    //     br 0                       (back to loop top)
    //   end
    // end
    emit(WasmVM::Instr::Block{std::nullopt});
    emit(WasmVM::Instr::Loop{std::nullopt});
    emitExpr(stmt.cond);
    emit(WasmVM::Instr::I32_eqz{});
    emit(WasmVM::Instr::Br_if{1});
    emitStmt(stmt.body);
    emit(WasmVM::Instr::Br{0});
    emit(WasmVM::Instr::End{});
    emit(WasmVM::Instr::End{});
}

void FunctionCodegen::emitForStmt(const wvmcc::parser::ForStmt& stmt) {
    // Push a scope so the init declaration is scoped to the for statement.
    symbolTable_.pushScope();
    if (stmt.init) {
        emitBlockItem(*stmt.init);
    }
    emit(WasmVM::Instr::Block{std::nullopt});
    emit(WasmVM::Instr::Loop{std::nullopt});
    if (stmt.cond) {
        emitExpr(*stmt.cond);
        emit(WasmVM::Instr::I32_eqz{});
        emit(WasmVM::Instr::Br_if{1});
    }
    emitStmt(stmt.body);
    if (stmt.step) {
        emitExpr(*stmt.step);
        emit(WasmVM::Instr::Drop{});
    }
    emit(WasmVM::Instr::Br{0});
    emit(WasmVM::Instr::End{});
    emit(WasmVM::Instr::End{});
    symbolTable_.popScope();
}

WasmVM::ValueType FunctionCodegen::getExprType(const wvmcc::parser::ExprPtr& expr) const {
    // Placeholder — defaults to i32 until a full type-inference pass exists.
    return WasmVM::ValueType::i32;
}

} // namespace wvmcc::codegen
