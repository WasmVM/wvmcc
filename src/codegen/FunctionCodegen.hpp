#pragma once

#include "TypeMap.hpp"
#include "SymbolTable.hpp"
#include "AddressTakenAnalyzer.hpp"
#include "../parser/AST.hpp"
#include "../parser/Semantic.hpp"
#include <WasmVM.hpp>
#include <vector>
#include <stack>
#include <unordered_set>

namespace wvmcc::codegen {

class FunctionCodegen {
public:
    FunctionCodegen(const TypeMap& typeMap, SymbolTable& symbolTable);

    // Generate code for a function definition
    WasmVM::WasmFunc generate(const wvmcc::parser::FunctionDefPtr& funcDef,
                              const wvmcc::parser::Semantic& semantic);

    // Allocate a local variable (returns Wasm local index)
    int allocLocal(const wvmcc::parser::TypeNodePtr& type, bool isAddressTaken = false);

    const std::unordered_set<std::string>& getAddressTakenNames() const {
        return addressTakenNames_;
    }

    bool hasAddressTakenVariables() const {
        return !addressTakenNames_.empty();
    }

    const std::vector<WasmVM::WasmInstr>& getInstructions() const {
        return instrBuffer_;
    }

    void emit(const WasmVM::WasmInstr& instr);

    void emitExpr(const wvmcc::parser::ExprPtr& expr, bool needLValue = false);

    void emitIntegerLiteral(const wvmcc::parser::IntegerLiteral& expr);
    void emitCharLiteral(const wvmcc::parser::CharLiteral& expr);
    void emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr);
    void emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr);
    void emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr);
    void emitCastExpr(const wvmcc::parser::CastExpr& expr);
    void emitCallExpr(const wvmcc::parser::CallExpr& expr);
    void emitMemberAccessExpr(const wvmcc::parser::MemberExpr& expr);
    void emitArrayIndexExpr(const wvmcc::parser::IndexExpr& expr);
    void emitCompoundLiteralExpr(const wvmcc::parser::CompoundLiteral& expr);

    void emitStmt(const wvmcc::parser::StmtPtr& stmt);
    void emitBlockItem(const wvmcc::parser::BlockItemPtr& item);

private:
    const TypeMap& typeMap_;
    SymbolTable& symbolTable_;

    std::vector<WasmVM::WasmInstr> instrBuffer_;
    std::vector<WasmVM::ValueType> localTypes_;

    int localIndexCounter_ = 0;
    std::stack<int> controlFlowStack_;
    int framePointerLocal_ = -1;
    size_t frameSize_ = 0;

    std::unordered_set<std::string> addressTakenNames_;

    int allocRawLocal(WasmVM::ValueType valType);
    std::vector<WasmVM::WasmInstr> generatePrologue();
    void generateEpilogue();

    void emitStringLiteral(const wvmcc::parser::StringLiteral& expr);

    void emitReturnStmt(const wvmcc::parser::ReturnStmt& stmt);
    void emitExprStmt(const wvmcc::parser::ExprStmt& stmt);
    void emitCompoundStmt(const wvmcc::parser::CompoundStmt& stmt);
    void emitIfStmt(const wvmcc::parser::IfStmt& stmt);
    void emitWhileStmt(const wvmcc::parser::WhileStmt& stmt);
    void emitForStmt(const wvmcc::parser::ForStmt& stmt);

    WasmVM::ValueType getExprType(const wvmcc::parser::ExprPtr& expr) const;
};

} // namespace wvmcc::codegen
