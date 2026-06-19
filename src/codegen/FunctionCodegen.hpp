#pragma once

#include <cstdint>
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

    // #79: positions within `instrBuffer_` where a function-pointer value
    // (`i64.const (funcptr-tag | table-slot)`) was emitted, paired with the
    // referenced function name. The linker rebases the embedded slot when it
    // merges per-TU funcref tables into one unified table.
    struct FuncPtrSite {
        size_t instrIdx;
        std::string funcName;
    };
    const std::vector<FuncPtrSite>& getFuncPtrSites() const { return funcPtrSites_; }

    void emit(const WasmVM::WasmInstr& instr);

    // Emit `unreachable` AND record an error diagnostic for an unhandled or
    // erroneous construct (design Step 5.1: no silent wrong code).
    void emitUnimplemented(const std::string& message,
                           std::optional<wvmcc::SourceSpan> span = std::nullopt);

    // Push a GlobalMem's mem[0] address: a baked `i64.const` for a locally
    // defined object, or `global.get` of the imported address-global for a
    // cross-TU `extern` reference (resolved by the linker).
    void emitGlobalMemAddr(const GlobalMem& gm);

    // needLValue=true: leave the address (i64) on the stack rather than the value.
    void emitExpr(const wvmcc::parser::ExprPtr& expr, bool needLValue = false);

    // Whether evaluating `expr` for its value leaves exactly one result on the
    // stack (false for void/struct-returning calls and void va_* builtins).
    bool exprLeavesValue(const wvmcc::parser::ExprPtr& expr);

    // C 6.5.1.1: pick the generic association whose type name is compatible with
    // the (lvalue-converted) controlling type, else the `default`. Returns the
    // selected expression, or null if nothing matches.
    wvmcc::parser::ExprPtr selectGenericAssociation(
        const wvmcc::parser::GenericSelectionExpr& g) const;

    void emitIntegerLiteral(const wvmcc::parser::IntegerLiteral& expr);
    void emitCharLiteral(const wvmcc::parser::CharLiteral& expr);
    void emitFloatLiteral(const wvmcc::parser::FloatLiteral& expr);
    void emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr, bool needLValue = false);
    void emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr);
    void emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr, bool needLValue = false);
    void emitPostfixUnaryExpr(const wvmcc::parser::PostfixUnaryExpr& expr, bool needLValue = false);
    void emitCastExpr(const wvmcc::parser::CastExpr& expr);
    // Reduce the i32 value on top of the stack to the width of `targetType` when
    // it is a narrow integer type (char/short) — masking for unsigned, sign-
    // extending for signed. No-op for wider types, non-integers, or _Bool (whose
    // 0/1 normalization is handled separately). Used by both explicit casts and
    // assignment/initialization conversions (6.3.1.3 / 6.5.16.1).
    void emitIntegerNarrow(const wvmcc::parser::TypeNodePtr& targetType);
    void emitCallExpr(const wvmcc::parser::CallExpr& expr);
    void emitMemberAccessExpr(const wvmcc::parser::MemberExpr& expr, bool needLValue = false);
    void emitArrayIndexExpr(const wvmcc::parser::IndexExpr& expr, bool needLValue = false);

    // Where an lvalue lives, for choosing how to load/store it. An access
    // rooted at a named object resolves to a *static* memory (mem[0] for a
    // file-scope GlobalMem, mem[1] for a shadow-stack MemoryLocal) and uses a
    // plain load/store with an untagged frame/static address. An access rooted
    // at a pointer value (deref/arrow/pointer-index anywhere in the chain) is
    // Dynamic: the pointer carries its memidx in the high nibble (see the
    // tagged-pointer model below), so the load/store must dispatch on that tag.
    enum class AddrKind { Mem0, Mem1, Dynamic };
    AddrKind addressKind(const wvmcc::parser::Expr* e);

    // Tagged-pointer model (memidx in bits [60:63], offset in [0:59]):
    //   &x / array-or-aggregate decay produce a pointer *value* whose high
    //   nibble is the object's memidx (mem[1] locals -> 1, mem[0] globals/heap
    //   -> 0). Frame/static addresses used for direct named access stay
    //   untagged. A deref through an opaque pointer dispatches on the nibble.
    static constexpr int    kMemidxShift = 60;
    static constexpr int64_t kPtrTagMask = (int64_t)0xF << 60;
    static constexpr int64_t kPtrOffMask = ~((int64_t)0xF << 60);
    // #79: a function pointer is a tagged i64 whose high nibble is this
    // sentinel (distinct from mem[0]=0 / mem[1]=1, so NULL=0 is never a valid
    // function pointer) and whose low bits hold a funcref-table slot, called via
    // call_indirect. Memory-storable, unlike a Wasm funcref.
    static constexpr int64_t kFuncPtrTag = (int64_t)0xF << 60;
    // Emit a function-pointer value for `name`: `i64.const (kFuncPtrTag | slot)`,
    // interning a funcref-table slot and recording a relocation site.
    void emitFuncPtrValue(const std::string& name);
    // OR the memidx tag (mem[1] only; mem[0]/Dynamic are no-ops) onto the i64
    // address currently on the operand stack.
    void emitApplyTag(AddrKind k);
    // Assuming a tagged i64 pointer is on the stack, emit a load of `type`
    // that masks off the tag and dispatches to mem[0] or mem[1] by the nibble.
    void emitTaggedLoad(const wvmcc::parser::TypeNodePtr& type);
    // Assuming [tagged-addr(i64), value(T)] are on the stack (addr pushed
    // first), emit a tag-dispatched store of `type`.
    void emitTaggedStore(const wvmcc::parser::TypeNodePtr& type);

    // #79: whether an indirect call through `callee` leaves a value on the
    // stack (false for a void-returning function pointer).
    bool indirectCallLeavesValue(const wvmcc::parser::ExprPtr& callee);

    void emitCompoundLiteralExpr(const wvmcc::parser::CompoundLiteral& expr, bool needLValue = false);

    // Copy `size` bytes from the address in `srcAddrLocal` (memory `srcMemidx`)
    // to the address in `dstAddrLocal` (memory `dstMemidx`), in 8/4/2/1-byte
    // chunks. Used for aggregate copy-initialization (`struct q = <rvalue>;`).
    void emitBytewiseCopy(int dstAddrLocal, uint8_t dstMemidx,
                          int srcAddrLocal, uint8_t srcMemidx, size_t size);

    void emitStmt(const wvmcc::parser::StmtPtr& stmt);
    void emitBlockItem(const wvmcc::parser::BlockItemPtr& item);

    // Emit a sequence of block items at one lexical level, lifting forward
    // gotos at this level into wrapping Blocks. Used by both function-body
    // emission and nested compound-statement emission.
    void emitItemsWithGotoLift(const std::vector<wvmcc::parser::BlockItemPtr>& items);

    // Whether `items` contain a label reached by a backward or non-local goto —
    // i.e. one the simple forward-goto lift cannot express. Such a block is
    // lowered via emitGotoDispatch (a state-machine dispatch loop) instead.
    bool topLevelNeedsGotoDispatch(const std::vector<wvmcc::parser::BlockItemPtr>& items);

    // Lower an arbitrary-goto block to a dispatch loop: split the items into
    // segments at labels, drive them by an i32 `state` local, and turn every
    // `goto L` into `state = seg(L); br $dispatch` and each label boundary into
    // a br_table dispatch target (WebAssembly has only structured control flow).
    void emitGotoDispatch(const std::vector<wvmcc::parser::BlockItemPtr>& items);

    // Active dispatch-loop context (set while inside emitGotoDispatch) so a
    // nested `goto` can branch back to the loop and re-dispatch.
    bool gotoDispatchActive_ = false;
    int gotoStateLocal_ = -1;       // i32 local holding the current segment id
    int gotoDispatchLoopDepth_ = 0; // currentBlockDepth_ at the dispatch loop
    int gotoDispatchExitDepth_ = 0; // currentBlockDepth_ at the enclosing exit block
    std::unordered_map<std::string, int> gotoLabelSeg_; // label name -> segment id

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
    // C scalar return type node, so emitReturnStmt can apply the conversion-as-
    // by-assignment to the return value (narrowing to char/short width, _Bool
    // normalization). Null for struct/void returns.
    wvmcc::parser::TypeNodePtr returnScalarType_;
    // Name of the function currently being generated, for the __func__
    // predefined identifier (C 6.4.2.2).
    std::string currentFunctionName_;

    std::unordered_set<std::string> addressTakenNames_;

    // Sites where a data-pointer i64.const was emitted (M2-E).
    std::vector<DataPtrSite> dataPtrSites_;
    std::vector<FuncPtrSite> funcPtrSites_;

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
