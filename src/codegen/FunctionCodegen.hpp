#pragma once

#include "TypeMap.hpp"
#include "SymbolTable.hpp"
#include "GlobalDataAllocator.hpp"
#include "AddressTakenAnalyzer.hpp"
#include "../common.hpp"
#include "../parser/AST.hpp"
#include "../parser/Semantic.hpp"
#include <WasmVM.hpp>
#include <vector>
#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace wvmcc::codegen {

class ModuleCodegen; // forward declaration to break circular include

// Tracks break/continue targets for nested loops and switch statements.
// breakDepth/continueDepth store the value of currentBlockDepth_ *after* the
// corresponding scope (Block/Loop) has been opened. The Br index from any
// emission point inside is then `currentBlockDepth_ - savedDepth`.
struct ControlFlowEntry {
    enum Kind { Loop, Switch } kind;
    int breakDepth;
    int continueDepth;
};

class FunctionCodegen {
public:
    FunctionCodegen(const TypeMap& typeMap, SymbolTable& symbolTable,
                    GlobalDataAllocator* dataAllocator = nullptr,
                    ModuleCodegen* moduleCg = nullptr,
                    const wvmcc::parser::Semantic* semantic = nullptr);

    // Generate code for a function definition
    WasmVM::WasmFunc generate(const wvmcc::parser::FunctionDefPtr& funcDef,
                              const wvmcc::parser::Semantic& semantic);

    // Allocate a local variable (returns Wasm local index or shadow-stack frame offset)
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

    const std::vector<wvmcc::Diagnostic>& getDiagnostics() const {
        return diagnostics_;
    }

    // M2-E: positions within `instrBuffer_` where a data-pointer i64.const
    // was emitted, paired with the mem[0] address pushed. ModuleCodegen
    // reads this after generate() to populate reloc.CODE.
    struct DataPtrSite {
        size_t instrIdx;
        size_t address;
    };
    const std::vector<DataPtrSite>& getDataPtrSites() const { return dataPtrSites_; }

    void emit(const WasmVM::WasmInstr& instr);

    // needLValue=true: leave the address (i64) on the stack rather than the value.
    void emitExpr(const wvmcc::parser::ExprPtr& expr, bool needLValue = false);

    void emitIntegerLiteral(const wvmcc::parser::IntegerLiteral& expr);
    void emitCharLiteral(const wvmcc::parser::CharLiteral& expr);
    void emitFloatLiteral(const wvmcc::parser::FloatLiteral& expr);
    void emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr, bool needLValue = false);
    void emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr);
    void emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr, bool needLValue = false);
    void emitCastExpr(const wvmcc::parser::CastExpr& expr);
    void emitCallExpr(const wvmcc::parser::CallExpr& expr);
    void emitMemberAccessExpr(const wvmcc::parser::MemberExpr& expr, bool needLValue = false);
    void emitArrayIndexExpr(const wvmcc::parser::IndexExpr& expr, bool needLValue = false);
    void emitCompoundLiteralExpr(const wvmcc::parser::CompoundLiteral& expr);

    void emitStmt(const wvmcc::parser::StmtPtr& stmt);
    void emitBlockItem(const wvmcc::parser::BlockItemPtr& item);

    // Emit a sequence of block items at one lexical level, lifting forward
    // gotos at this level into wrapping Blocks. Used by both function-body
    // emission and nested compound-statement emission.
    void emitItemsWithGotoLift(const std::vector<wvmcc::parser::BlockItemPtr>& items);

    // For testing: force the frame-pointer local to a specific index.
    // In production, generate() sets this automatically when address-taken vars exist.
    void forceFramePointerLocal(int idx) { framePointerLocal_ = idx; }
    int getFramePointerLocal() const { return framePointerLocal_; }

private:
    const TypeMap& typeMap_;
    SymbolTable& symbolTable_;
    GlobalDataAllocator* dataAllocator_;
    ModuleCodegen* moduleCg_;
    const wvmcc::parser::Semantic* semantic_;

    std::vector<WasmVM::WasmInstr> instrBuffer_;
    std::vector<WasmVM::ValueType> localTypes_;

    int localIndexCounter_ = 0;
    std::stack<ControlFlowEntry> controlFlowStack_;
    int currentBlockDepth_ = 0;            // # currently open block/loop/if scopes
    std::vector<wvmcc::Diagnostic> diagnostics_;
    int framePointerLocal_ = -1;
    size_t frameSize_ = 0;

    // ABI: hidden first parameter for struct-returning functions (-1 if not struct return)
    int hiddenRetPtrLocal_ = -1;
    // ABI: hidden trailing parameter for variadic callees (spill-base ptr, -1 if not variadic)
    int vaArgsPtrLocal_ = -1;
    // C return type when function returns a struct (used by emitReturnStmt)
    wvmcc::parser::TypeNodePtr returnTypeNode_;
    // Wasm result type so emitReturnStmt can coerce e.g. an `int` value
    // to i64 when the function signature says ssize_t/long. Unset for
    // void-returning functions (no return value to coerce).
    std::optional<WasmVM::ValueType> returnWasmType_;

    std::unordered_set<std::string> addressTakenNames_;

    // Sites where a data-pointer i64.const was emitted (M2-E).
    std::vector<DataPtrSite> dataPtrSites_;

    int allocRawLocal(WasmVM::ValueType valType);
    std::vector<WasmVM::WasmInstr> generatePrologue();
    void generateEpilogue();

    void emitStringLiteral(const wvmcc::parser::StringLiteral& expr);
    void emitStructCopyToHiddenPtr(const wvmcc::parser::ExprPtr& srcExpr);

    // __builtin_va_start / __builtin_va_arg / __builtin_va_end / __builtin_va_copy
    void emitVaBuiltin(const std::string& name, const wvmcc::parser::CallExpr& expr);

    // Emit a (possibly designated) initializer-list assigning into the storage
    // at `baseAddrLocal + 0`. memidx selects mem[0] (static / heap) vs mem[1]
    // (shadow stack).
    void emitListInitializer(int baseAddrLocal,
                             const wvmcc::parser::TypeNodePtr& type,
                             const wvmcc::parser::InitializerPtr& init,
                             uint8_t memidx);

    void emitReturnStmt(const wvmcc::parser::ReturnStmt& stmt);
    void emitExprStmt(const wvmcc::parser::ExprStmt& stmt);
    void emitCompoundStmt(const wvmcc::parser::CompoundStmt& stmt);
    void emitIfStmt(const wvmcc::parser::IfStmt& stmt);
    void emitWhileStmt(const wvmcc::parser::WhileStmt& stmt);
    void emitForStmt(const wvmcc::parser::ForStmt& stmt);
    void emitDoWhileStmt(const wvmcc::parser::DoWhileStmt& stmt);
    void emitSwitchStmt(const wvmcc::parser::SwitchStmt& stmt);
    void emitBreakStmt(const wvmcc::parser::BreakStmt& stmt);
    void emitContinueStmt(const wvmcc::parser::ContinueStmt& stmt);
    void emitGotoStmt(const wvmcc::parser::GotoStmt& stmt);
    void emitLabelStmt(const wvmcc::parser::LabelStmt& stmt);

    // Control-flow helpers: push/pop ControlFlowEntry as scopes are opened.
    // pushLoop() takes explicit absolute depths for break / continue targets
    // (the value of currentBlockDepth_ right after the corresponding Block /
    // Loop was opened). pushSwitch() expects the outer break-target Block
    // has already been emitted.
    void pushLoop(int breakDepthAtOpen, int continueDepthAtOpen);
    void pushSwitch();
    void popControlFlow();
    // Br index for break/continue from the current emission point. continue
    // skips Switch entries to find the nearest enclosing loop.
    WasmVM::index_t breakDepth() const;
    WasmVM::index_t continueDepth() const;

    WasmVM::ValueType getExprType(const wvmcc::parser::ExprPtr& expr) const;

    // Return the C TypeNode for an expression (best-effort; may return nullptr).
    wvmcc::parser::TypeNodePtr getExprTypeNode(const wvmcc::parser::ExprPtr& expr) const;
};

} // namespace wvmcc::codegen
