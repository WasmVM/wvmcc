#pragma once

#include "TypeMap.hpp"
#include "SymbolTable.hpp"
#include "../parser/AST.hpp"
#include "../parser/Semantic.hpp"
#include <WasmVM.hpp>
#include <vector>
#include <stack>

namespace wvmcc::codegen {

class FunctionCodegen {
public:
    FunctionCodegen(const TypeMap& typeMap, SymbolTable& symbolTable);
    
    // Generate code for a function definition
    WasmVM::WasmFunc generate(const wvmcc::parser::FunctionDefPtr& funcDef, 
                              const wvmcc::parser::Semantic& semantic);
    
    // Allocate a local variable
    int allocLocal(const wvmcc::parser::TypeNodePtr& type, bool isAddressTaken = false);
    
    // Emit an instruction
    void emit(const WasmVM::WasmInstr& instr);
    
    // Emit an expression
    void emitExpr(const wvmcc::parser::ExprPtr& expr, bool needLValue = false);
    
    // Emit a statement
    void emitStmt(const wvmcc::parser::StmtPtr& stmt);
    
    // Emit a block item
    void emitBlockItem(const wvmcc::parser::BlockItemPtr& item);
    
private:
    const TypeMap& typeMap_;
    SymbolTable& symbolTable_;
    
    // Function instruction buffer
    std::vector<WasmVM::WasmInstr> instrBuffer_;
    
    // Local variable allocation tracking
    int localIndexCounter_ = 0;
    
    // Stack for control flow tracking
    std::stack<int> controlFlowStack_;
    
    // Frame pointer local index
    int framePointerLocal_ = -1;
    
    // Helper functions for expression emission
    void emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr);
    void emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr);
    void emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr);
    void emitIntegerLiteral(const wvmcc::parser::IntegerLiteral& expr);
    void emitStringLiteral(const wvmcc::parser::StringLiteral& expr);
    
    // Helper functions for statement emission
    void emitReturnStmt(const wvmcc::parser::ReturnStmt& stmt);
    void emitExprStmt(const wvmcc::parser::ExprStmt& stmt);
    void emitCompoundStmt(const wvmcc::parser::CompoundStmt& stmt);
    void emitIfStmt(const wvmcc::parser::IfStmt& stmt);
    void emitWhileStmt(const wvmcc::parser::WhileStmt& stmt);
    void emitForStmt(const wvmcc::parser::ForStmt& stmt);
    
    // Helper to get Wasm type for an expression
    WasmVM::ValueType getExprType(const wvmcc::parser::ExprPtr& expr) const;
};

} // namespace wvmcc::codegen