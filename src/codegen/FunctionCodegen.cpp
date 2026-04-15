#include "FunctionCodegen.hpp"
#include "AddressTakenAnalyzer.hpp"
#include <stdexcept>

namespace wvmcc::codegen {

FunctionCodegen::FunctionCodegen(const TypeMap& typeMap, SymbolTable& symbolTable)
    : typeMap_(typeMap), symbolTable_(symbolTable) {}

WasmVM::WasmFunc FunctionCodegen::generate(const wvmcc::parser::FunctionDefPtr& funcDef,
                                             const wvmcc::parser::Semantic& semantic) {
    WasmVM::WasmFunc func;
    
    // Run AddressTakenAnalyzer at the start to identify address-taken variables
    AddressTakenAnalyzer analyzer;
    auto addressTakenNames = analyzer.analyze(funcDef);
    addressTakenNames_ = addressTakenNames;
    
    // Initialize function with empty body for now
    func.body = instrBuffer_;
    
    // For now, just return an empty function - this will be expanded in later phases
    return func;
}

int FunctionCodegen::allocLocal(const wvmcc::parser::TypeNodePtr& type, bool isAddressTaken) {
    // In a real implementation, this would track local variable allocation
    // For now, just return a counter
    // TODO: Implement proper local variable tracking with MemoryLocal support
    
    if (isAddressTaken) {
        // For address-taken variables, we would need to allocate them as MemoryLocal
        // This is a placeholder - in a real implementation, we would:
        // 1. Track these variables with their frame offsets
        // 2. Generate appropriate prologue/epilogue code for stack management
        return localIndexCounter_++;
    } else {
        // For regular variables, continue with normal allocation
        return localIndexCounter_++;
    }
}

void FunctionCodegen::emit(const WasmVM::WasmInstr& instr) {
    instrBuffer_.push_back(instr);
}

void FunctionCodegen::emitExpr(const wvmcc::parser::ExprPtr& expr, bool needLValue) {
    if (!expr) return;
    
    std::visit([this](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, wvmcc::parser::IntegerLiteral>) {
            emitIntegerLiteral(e);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::IdentifierExpr>) {
            emitIdentifierExpr(e);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::BinaryExpr>) {
            emitBinaryExpr(e);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::UnaryExpr>) {
            emitUnaryExpr(e);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::CastExpr>) {
            emitCastExpr(e);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::CharLiteral>) {
            emitCharLiteral(e);
        } else {
            // For unhandled expression types, emit unreachable
            emit(WasmVM::Instr::Unreachable{});
        }
    }, expr->expr);
}

void FunctionCodegen::emitIntegerLiteral(const wvmcc::parser::IntegerLiteral& expr) {
    // Emit appropriate Wasm constant based on value size
    if (expr.value >= std::numeric_limits<int32_t>::min() && expr.value <= std::numeric_limits<int32_t>::max()) {
        // For 32-bit values, emit I32_const
        emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)expr.value});
    } else {
        // For 64-bit values, emit I64_const
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)expr.value});
    }
}

void FunctionCodegen::emitCharLiteral(const wvmcc::parser::CharLiteral& expr) {
    // Char literals are 8-bit values, so emit as I32_const (promoted to 32-bit)
    emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)expr.value});
}

void FunctionCodegen::emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr) {
    // Look up the identifier in symbol table
    auto symbolInfo = symbolTable_.lookup(expr.name);
    if (!symbolInfo) {
        // Handle unknown identifier - emit unreachable
        emit(WasmVM::Instr::Unreachable{});
        return;
    }
    
    // Handle different symbol types
    std::visit([this](const auto& info) {
        using T = std::decay_t<decltype(info)>;
        if constexpr (std::is_same_v<T, ScalarLocal>) {
            // Local variable - emit Local_get
            emit(WasmVM::Instr::Local_get{info.localIndex});
        } else if constexpr (std::is_same_v<T, GlobalScalar>) {
            // Global scalar - emit Global_get
            emit(WasmVM::Instr::Global_get{info.globalIndex});
        } else {
            // For other symbol types, emit unreachable
            emit(WasmVM::Instr::Unreachable{});
        }
    }, *symbolInfo);
}

void FunctionCodegen::emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr) {
    // Emit right-hand side first, then left-hand side
    emitExpr(expr.rhs, false);
    emitExpr(expr.lhs, false);
    
    // Handle different operators
    if (expr.op == "+") {
        // Determine type and emit appropriate add instruction
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_add{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_add{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "-") {
        // Determine type and emit appropriate sub instruction
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_sub{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_sub{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "*") {
        // Determine type and emit appropriate mul instruction
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_mul{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_mul{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "/") {
        // Determine type and emit appropriate div instruction
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_div_s{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_div_s{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "==") {
        // Comparison - always returns i32
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_eq{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_eq{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "!=") {
        // Comparison - always returns i32
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_ne{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_ne{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "<") {
        // Comparison - always returns i32
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_lt_s{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_lt_s{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == ">") {
        // Comparison - always returns i32
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_gt_s{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_gt_s{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "<=") {
        // Comparison - always returns i32
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_le_s{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_le_s{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == ">=") {
        // Comparison - always returns i32
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_ge_s{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_ge_s{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "&") {
        // Bitwise AND
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_and{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_and{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "|") {
        // Bitwise OR
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_or{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_or{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "^") {
        // Bitwise XOR
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_xor{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_xor{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "<<") {
        // Left shift
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_shl{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_shl{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == ">>") {
        // Right shift (signed)
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_shr_s{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_shr_s{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "%") {
        // Modulo
        auto lhsType = getExprType(expr.lhs);
        if (lhsType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_rem_s{});
        } else if (lhsType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_rem_s{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else {
        // For unhandled operators, emit unreachable
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr) {
    // Emit the operand first
    emitExpr(expr.rhs, false);
    
    // Handle different unary operators
    if (expr.op == "-") {
        // Unary minus - negate the value
        auto exprType = getExprType(expr.rhs);
        if (exprType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_sub{});
        } else if (exprType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_sub{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "~") {
        // Bitwise NOT - invert all bits
        auto exprType = getExprType(expr.rhs);
        if (exprType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_not{});
        } else if (exprType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_not{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "!") {
        // Logical NOT - convert to i32 result
        auto exprType = getExprType(expr.rhs);
        if (exprType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_eqz{});
        } else if (exprType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_eqz{});
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    } else if (expr.op == "+") {
        // Unary plus - no change needed, just pass through
        // (This is a no-op in Wasm)
    } else {
        // For unhandled unary operators, emit unreachable
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitCastExpr(const wvmcc::parser::CastExpr& expr) {
    // Emit the expression to cast
    emitExpr(expr.expr, false);
    
    // Handle different cast conversions
    auto targetType = typeMap_.toWasmType(expr.type);
    auto sourceType = getExprType(expr.expr);
    
    // Handle type conversions
    if (sourceType == WasmVM::ValueType::i32 && targetType == WasmVM::ValueType::i64) {
        // i32 to i64 (sign-extend)
        emit(WasmVM::Instr::I64_extend_i32_s{});
    } else if (sourceType == WasmVM::ValueType::i64 && targetType == WasmVM::ValueType::i32) {
        // i64 to i32 (wrap)
        emit(WasmVM::Instr::I32_wrap_i64{});
    } else if (sourceType == WasmVM::ValueType::f32 && targetType == WasmVM::ValueType::i64) {
        // f32 to i64 (convert)
        emit(WasmVM::Instr::I64_convert_f32_s{});
    } else if (sourceType == WasmVM::ValueType::f64 && targetType == WasmVM::ValueType::i64) {
        // f64 to i64 (convert)
        emit(WasmVM::Instr::I64_convert_f64_s{});
    } else if (sourceType == WasmVM::ValueType::i64 && targetType == WasmVM::ValueType::f32) {
        // i64 to f32 (convert)
        emit(WasmVM::Instr::F32_convert_i64_s{});
    } else if (sourceType == WasmVM::ValueType::i32 && targetType == WasmVM::ValueType::f64) {
        // i32 to f64 (convert)
        emit(WasmVM::Instr::F64_convert_i32_s{});
    } else {
        // For unhandled conversions, emit unreachable
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitStmt(const wvmcc::parser::StmtPtr& stmt) {
    // Placeholder implementation - this will be expanded in later phases
    if (!stmt) return;
    
    // For now, emit a simple unreachable instruction for any statement
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitBlockItem(const wvmcc::parser::BlockItemPtr& item) {
    // Placeholder implementation - this will be expanded in later phases
    if (!item) return;
    
    // For now, emit a simple unreachable instruction for any block item
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitReturnStmt(const wvmcc::parser::ReturnStmt& stmt) {
    // Placeholder implementation
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitExprStmt(const wvmcc::parser::ExprStmt& stmt) {
    // Placeholder implementation
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitCompoundStmt(const wvmcc::parser::CompoundStmt& stmt) {
    // Placeholder implementation
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitIfStmt(const wvmcc::parser::IfStmt& stmt) {
    // Placeholder implementation
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitWhileStmt(const wvmcc::parser::WhileStmt& stmt) {
    // Placeholder implementation
    emit(WasmVM::Instr::Unreachable{});
}

void FunctionCodegen::emitForStmt(const wvmcc::parser::ForStmt& stmt) {
    // Placeholder implementation
    emit(WasmVM::Instr::Unreachable{});
}

WasmVM::ValueType FunctionCodegen::getExprType(const wvmcc::parser::ExprPtr& expr) const {
    // Placeholder implementation - return a default value
    return WasmVM::ValueType::i32;
}

} // namespace wvmcc::codegen