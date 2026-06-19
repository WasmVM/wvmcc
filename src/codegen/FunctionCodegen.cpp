#include <cstdint>
#include "FunctionCodegen.hpp"
#include "ModuleCodegen.hpp"
#include "AddressTakenAnalyzer.hpp"
#include "../parser/ConstExprEval.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <limits>
#include <unordered_map>

namespace wvmcc::codegen {

namespace {

// C "usual arithmetic conversions" subset over Wasm value types.
// f64 dominates everything; f32 dominates ints; i64 dominates i32.
WasmVM::ValueType arithCommonType(WasmVM::ValueType lhs, WasmVM::ValueType rhs) {
    if (lhs == WasmVM::ValueType::f64 || rhs == WasmVM::ValueType::f64) return WasmVM::ValueType::f64;
    if (lhs == WasmVM::ValueType::f32 || rhs == WasmVM::ValueType::f32) return WasmVM::ValueType::f32;
    if (lhs == WasmVM::ValueType::i64 || rhs == WasmVM::ValueType::i64) return WasmVM::ValueType::i64;
    return WasmVM::ValueType::i32;
}

} // namespace

// Emit a conversion instruction that turns `from` into `to`. Caller guarantees
// the value of type `from` is on top of the Wasm stack. No-op if from == to.
// (Signed conversions; unsigned variants are not yet exposed by the type system.)
static void emitConvert(FunctionCodegen* fc, WasmVM::ValueType from, WasmVM::ValueType to) {
    if (from == to) return;
    using VT = WasmVM::ValueType;
    if (from == VT::i32 && to == VT::i64) { fc->emit(WasmVM::Instr::I64_extend_i32_s{}); return; }
    if (from == VT::i64 && to == VT::i32) { fc->emit(WasmVM::Instr::I32_wrap_i64{}); return; }
    if (from == VT::i32 && to == VT::f32) { fc->emit(WasmVM::Instr::F32_convert_i32_s{}); return; }
    if (from == VT::i32 && to == VT::f64) { fc->emit(WasmVM::Instr::F64_convert_i32_s{}); return; }
    if (from == VT::i64 && to == VT::f32) { fc->emit(WasmVM::Instr::F32_convert_i64_s{}); return; }
    if (from == VT::i64 && to == VT::f64) { fc->emit(WasmVM::Instr::F64_convert_i64_s{}); return; }
    if (from == VT::f32 && to == VT::f64) { fc->emit(WasmVM::Instr::F64_promote_f32{}); return; }
    if (from == VT::f64 && to == VT::f32) { fc->emit(WasmVM::Instr::F32_demote_f64{}); return; }
    // Floating -> integer truncates toward zero (6.3.1.4).
    if (from == VT::f32 && to == VT::i32) { fc->emit(WasmVM::Instr::I32_trunc_sat_f32_s{}); return; }
    if (from == VT::f64 && to == VT::i32) { fc->emit(WasmVM::Instr::I32_trunc_sat_f64_s{}); return; }
    if (from == VT::f32 && to == VT::i64) { fc->emit(WasmVM::Instr::I64_trunc_sat_f32_s{}); return; }
    if (from == VT::f64 && to == VT::i64) { fc->emit(WasmVM::Instr::I64_trunc_sat_f64_s{}); return; }
    fc->emitUnimplemented("codegen: unsupported value-type conversion");
}


FunctionCodegen::FunctionCodegen(const TypeMap& typeMap, SymbolTable& symbolTable,
                                 GlobalDataAllocator* dataAllocator,
                                 ModuleCodegen* moduleCg,
                                 const wvmcc::parser::Semantic* semantic)
    : typeMap_(typeMap), symbolTable_(symbolTable), dataAllocator_(dataAllocator),
      moduleCg_(moduleCg), semantic_(semantic) {}

WasmVM::WasmFunc FunctionCodegen::generate(const wvmcc::parser::FunctionDefPtr& funcDef,
                                             const wvmcc::parser::Semantic& semantic) {
    WasmVM::WasmFunc func;

    // Capture the function's name for the __func__ predefined identifier.
    currentFunctionName_.clear();
    for (auto cur = funcDef->declarator; cur;
         cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (!cur->id.name.empty()) { currentFunctionName_ = cur->id.name; break; }
    }

    AddressTakenAnalyzer analyzer;
    addressTakenNames_ = analyzer.analyze(funcDef);
    // The analyzer also flags `&funcname`. Function names don't need a frame
    // pointer / shadow-stack slot — Phase 4 routes them through funcref tables
    // instead. Drop any name that resolves to a known function.
    for (auto it = addressTakenNames_.begin(); it != addressTakenNames_.end(); ) {
        if (symbolTable_.lookupFunction(*it).has_value()) it = addressTakenNames_.erase(it);
        else ++it;
    }

    // Detect struct return: struct/union base type AND no pointer in the declarator chain
    // before the function declarator.
    bool hasReturnPointer = false;
    for (auto cur = funcDef->declarator; cur;
         cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (cur->kind == wvmcc::parser::Declarator::Kind::Function
            || cur->kind == wvmcc::parser::Declarator::Kind::Identifier) break;
        if (cur->kind == wvmcc::parser::Declarator::Kind::Pointer) { hasReturnPointer = true; break; }
    }
    if (!hasReturnPointer) {
        for (const auto& ts : funcDef->specifiers.typeSpecifiers) {
            if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                && ts.su) {
                auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                node->kind = (ts.su->kind == wvmcc::parser::StructOrUnionSpecifier::Kind::Struct)
                             ? wvmcc::parser::TypeNode::Kind::Struct
                             : wvmcc::parser::TypeNode::Kind::Union;
                node->su = ts.su;
                returnTypeNode_ = node;
                break;
            }
        }
    }
    bool isStructRet = returnTypeNode_ != nullptr;

    // Compute the Wasm result type so emitReturnStmt can coerce the
    // returned expression to match the function signature (e.g. `int`
    // → i64 when the function returns `ssize_t`).
    if (!isStructRet && semantic_) {
        // Mirror ModuleCodegen::buildReturnTypeNode (walk past the
        // Identifier, collect Pointer/Array wraps).
        auto baseType = semantic_->canonicalTypeRepr(
            funcDef->specifiers, nullptr);
        std::vector<wvmcc::parser::Declarator::Kind> quals;
        bool sawId = false;
        for (auto cur = funcDef->declarator; cur;
             cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
            if (sawId && (cur->kind == wvmcc::parser::Declarator::Kind::Pointer
                          || cur->kind == wvmcc::parser::Declarator::Kind::Array)) {
                quals.push_back(cur->kind);
            }
            if (cur->kind == wvmcc::parser::Declarator::Kind::Identifier) sawId = true;
        }
        for (auto it = quals.rbegin(); it != quals.rend(); ++it) {
            if (*it == wvmcc::parser::Declarator::Kind::Pointer) {
                auto p = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                p->kind = wvmcc::parser::TypeNode::Kind::Pointer;
                p->pointee = baseType;
                baseType = p;
            }
        }
        if (baseType) {
            bool isVoid = baseType->kind == wvmcc::parser::TypeNode::Kind::Builtin
                          && !baseType->simple.empty()
                          && baseType->simple[0]
                             == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void;
            if (!isVoid) { returnWasmType_ = typeMap_.toWasmType(baseType); returnScalarType_ = baseType; }
        }
    }

    // Parameters occupy local indices 0..n-1 and are not in func.locals.
    symbolTable_.pushScope();
    int paramIdx = 0;
    if (isStructRet) {
        hiddenRetPtrLocal_ = paramIdx++; // param 0 is the hidden sret pointer
    }
    // C `(void)` parameter list means zero parameters — skip the synthetic
    // void parameter so its slot doesn't shift the local index space.
    auto isVoidParamList = [](const std::vector<wvmcc::parser::Parameter>& ps) {
        if (ps.size() != 1) return false;
        const auto& p = ps[0];
        if (p.declarator) return false;
        for (const auto& ts : p.specifiers.typeSpecifiers) {
            if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                && ts.simple.size() == 1
                && ts.simple[0] == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void) {
                return true;
            }
        }
        return false;
    };
    const bool skipVoidParams = isVoidParamList(funcDef->params);
    for (const auto& param : funcDef->params) {
        if (skipVoidParams) break;
        if (param.declarator) {
            std::string pname;
            // Walk the declarator chain to find the bound identifier (the
            // outermost layer is empty for nested forms like `int (*op)(int,int)`).
            for (auto cur = param.declarator; cur;
                 cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
                if (!cur->id.name.empty()) { pname = cur->id.name; break; }
            }
            if (!pname.empty()) {
                wvmcc::parser::TypeNodePtr paramType;
                if (semantic_) {
                    // Use canonicalTypeRepr so typedef-named parameter types
                    // (e.g. `FILE *f` where `typedef struct FILE FILE;`)
                    // resolve to their underlying struct — otherwise member
                    // access on the parameter can't find field types and a
                    // pointer field wrongly defaults to i32. Falls back to
                    // buildTypeFromDeclaration internally when no typedef.
                    paramType = semantic_->canonicalTypeRepr(param.specifiers, param.declarator);
                }
                if (!paramType) {
                    for (const auto& ts : param.specifiers.typeSpecifiers) {
                        if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                            && !ts.simple.empty()) {
                            auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                            node->kind = wvmcc::parser::TypeNode::Kind::Builtin;
                            node->simple = ts.simple;
                            paramType = node;
                            break;
                        } else if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                                   && ts.su) {
                            auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                            node->kind = (ts.su->kind == wvmcc::parser::StructOrUnionSpecifier::Kind::Struct)
                                         ? wvmcc::parser::TypeNode::Kind::Struct
                                         : wvmcc::parser::TypeNode::Kind::Union;
                            node->su = ts.su;
                            paramType = node;
                            break;
                        }
                    }
                    if (paramType && param.declarator->inner.has_value()
                        && *param.declarator->inner
                        && (*param.declarator->inner)->kind == wvmcc::parser::Declarator::Kind::Pointer) {
                        auto ptrNode = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                        ptrNode->kind = wvmcc::parser::TypeNode::Kind::Pointer;
                        ptrNode->pointee = paramType;
                        paramType = ptrNode;
                    }
                }
                ScalarLocal info;
                info.type = paramType;
                info.isAddressTaken = false;
                info.localIndex = paramIdx;
                symbolTable_.define(pname, info);
            }
        }
        ++paramIdx;
    }
    // Variadic callees receive a hidden trailing i64 spill-base pointer.
    // It occupies the next param slot but has no C-level name.
    if (funcDef->isVariadic) {
        vaArgsPtrLocal_ = paramIdx++;
    }
    localIndexCounter_ = paramIdx; // locals start after params

    // Allocate frame-pointer local early so its index is fixed; frameSize_ is
    // computed lazily as address-taken variables are encountered.
    if (!addressTakenNames_.empty()) {
        framePointerLocal_ = allocRawLocal(WasmVM::ValueType::i64);
    }

    emitItemsWithGotoLift(funcDef->body);

    symbolTable_.popScope();

    // Wrap body with shadow-stack prologue/epilogue when needed.
    if (framePointerLocal_ != -1 && frameSize_ > 0) {
        auto prologue = generatePrologue();
        // Append epilogue at end for void/fallthrough paths.
        generateEpilogue();
        size_t prologueLen = prologue.size();
        prologue.insert(prologue.end(), instrBuffer_.begin(), instrBuffer_.end());
        instrBuffer_ = std::move(prologue);
        // The prologue was prepended; every previously-recorded data-pointer
        // site (M2-E) was indexed against the pre-prologue position. Shift
        // them so they still point at the same `i64.const` instructions in
        // the final body the linker will see.
        for (auto& site : dataPtrSites_) {
            site.instrIdx += prologueLen;
        }
        for (auto& site : funcPtrSites_) {
            site.instrIdx += prologueLen;
        }
    }

    // A non-void function may fall off the end (e.g. `main` with no explicit
    // return — 5.1.2.2.3 — or a switch whose cases all return). Wasm still
    // requires a result value on the stack at the function End, so push a
    // default zero of the return type. On paths that already returned this is
    // unreachable and harmless.
    if (returnWasmType_.has_value()) {
        switch (*returnWasmType_) {
            case WasmVM::ValueType::i32: emit(WasmVM::Instr::I32_const{0}); break;
            case WasmVM::ValueType::i64: emit(WasmVM::Instr::I64_const{0}); break;
            case WasmVM::ValueType::f32: emit(WasmVM::Instr::F32_const{0}); break;
            case WasmVM::ValueType::f64: emit(WasmVM::Instr::F64_const{0}); break;
            default: break;
        }
    }

    instrBuffer_.push_back(WasmVM::Instr::End{});
    func.locals = localTypes_;
    func.body = instrBuffer_;
    return func;
}

int FunctionCodegen::allocRawLocal(WasmVM::ValueType valType) {
    int idx = localIndexCounter_++;
    localTypes_.push_back(valType);
    return idx;
}

int FunctionCodegen::allocLocal(const wvmcc::parser::TypeNodePtr& type, bool isAddressTaken) {
    if (isAddressTaken) {
        // Lazily allocate the frame-pointer local the first time a shadow-stack slot is needed.
        // (generate() pre-allocates it when address-taken vars are known, but struct variables
        //  that are always memory-resident may require it even without explicit address-of.)
        if (framePointerLocal_ == -1) {
            framePointerLocal_ = allocRawLocal(WasmVM::ValueType::i64);
        }
        size_t align = type ? typeMap_.byteAlignment(type) : 1;
        size_t size  = type ? typeMap_.byteSize(type)      : 4;
        if (align > 1) {
            frameSize_ = (frameSize_ + align - 1) & ~(align - 1);
        }
        int offset = (int)frameSize_;
        frameSize_ += size;
        return offset;
    }
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
    switch (instr.opcode) {
    case WasmVM::Opcode::Block:
    case WasmVM::Opcode::Loop:
    case WasmVM::Opcode::If:
        ++currentBlockDepth_;
        break;
    case WasmVM::Opcode::End:
        --currentBlockDepth_;  // may go negative for the function-body End; harmless
        break;
    default:
        break;
    }
}

// Trap on an unhandled/erroneous construct AND record an error diagnostic.
// Design contract (lowering-plan.md Step 5.1): no silent wrong code — every
// unimplemented emitExpr/emitStmt branch must surface a diagnostic, not just
// emit a bare `unreachable` that the validator later rejects opaquely.
void FunctionCodegen::emitUnimplemented(const std::string& message,
                                        std::optional<wvmcc::SourceSpan> span) {
    emit(WasmVM::Instr::Unreachable{});
    wvmcc::Diagnostic d;
    d.severity = wvmcc::Diagnostic::Severity::Error;
    d.message = message;
    d.span = span;
    diagnostics_.push_back(std::move(d));
}

void FunctionCodegen::emitGlobalMemAddr(const GlobalMem& gm) {
    if (gm.isImport) {
        // Cross-TU extern: address carried by an imported Wasm global.
        emit(WasmVM::Instr::Global_get{(WasmVM::index_t)gm.importGlobalIndex});
    } else {
        // A file-scope object's absolute mem[0] address. Record the site so the
        // linker rebases this constant when the TU's data is relocated (M2-L8)
        // — otherwise BSS/zero-init globals (e.g. malloc's __heap_offset, which
        // has no data segment) collide with other TUs' data.
        dataPtrSites_.push_back({instrBuffer_.size(), gm.address});
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)gm.address});
    }
}

// #79: emit a function-pointer value for `name` — a tagged i64 carrying the
// function's funcref-table slot (high nibble = kFuncPtrTag, low bits = slot),
// called through `call_indirect`. The slot is interned lazily; the embedded
// constant is recorded as a relocation site so the linker can rebase it when
// per-TU funcref tables are merged into one.
void FunctionCodegen::emitFuncPtrValue(const std::string& name) {
    size_t slot = moduleCg_ ? moduleCg_->internFuncTableSlot(name) : 0;
    funcPtrSites_.push_back({instrBuffer_.size(), name});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)(kFuncPtrTag | (int64_t)slot)});
}

void FunctionCodegen::pushLoop(int breakDepthAtOpen, int continueDepthAtOpen) {
    ControlFlowEntry e;
    e.kind = ControlFlowEntry::Loop;
    e.breakDepth = breakDepthAtOpen;
    e.continueDepth = continueDepthAtOpen;
    controlFlowStack_.push(e);
}

void FunctionCodegen::pushSwitch() {
    ControlFlowEntry e;
    e.kind = ControlFlowEntry::Switch;
    e.breakDepth    = currentBlockDepth_;       // outer break block, after open
    e.continueDepth = -1;
    controlFlowStack_.push(e);
}

void FunctionCodegen::popControlFlow() {
    if (!controlFlowStack_.empty()) controlFlowStack_.pop();
}

WasmVM::index_t FunctionCodegen::breakDepth() const {
    if (controlFlowStack_.empty()) return 0;
    return (WasmVM::index_t)(currentBlockDepth_ - controlFlowStack_.top().breakDepth);
}

WasmVM::index_t FunctionCodegen::continueDepth() const {
    // Continue skips Switch entries to find the nearest enclosing Loop
    // (C semantics: `continue` inside a switch refers to the enclosing loop).
    auto stk = controlFlowStack_;
    while (!stk.empty()) {
        const auto& e = stk.top();
        if (e.kind == ControlFlowEntry::Loop) {
            return (WasmVM::index_t)(currentBlockDepth_ - e.continueDepth);
        }
        stk.pop();
    }
    return 0;
}

std::vector<WasmVM::WasmInstr> FunctionCodegen::generatePrologue() {
    // shadow-stack frame setup. The frame pointer is the *new* SP (low end of
    // the frame), so memory-resident locals at frameOffset `o` live at
    // `fp + o` for `o in [0, frameSize)`.
    //
    //   global.get __stack_pointer   ; push current SP (high end)
    //   i64.const  frameSize
    //   i64.sub                      ; new SP = old SP - frameSize
    //   local.tee  fp_local          ; fp = new SP, keep on stack
    //   global.set __stack_pointer   ; update SP
    constexpr WasmVM::index_t kStackPtrIdx = 0;
    std::vector<WasmVM::WasmInstr> p;
    p.push_back(WasmVM::Instr::Global_get{kStackPtrIdx});
    p.push_back(WasmVM::Instr::I64_const{(WasmVM::i64_t)frameSize_});
    p.push_back(WasmVM::Instr::I64_sub{});
    p.push_back(WasmVM::Instr::Local_tee{(WasmVM::index_t)framePointerLocal_});
    p.push_back(WasmVM::Instr::Global_set{kStackPtrIdx});
    return p;
}

void FunctionCodegen::generateEpilogue() {
    // shadow-stack frame teardown: SP = fp + frameSize (restore old SP).
    constexpr WasmVM::index_t kStackPtrIdx = 0;
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)frameSize_});
    emit(WasmVM::Instr::I64_add{});
    emit(WasmVM::Instr::Global_set{kStackPtrIdx});
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
    case K::Float:
        emitFloatLiteral(static_cast<const wvmcc::parser::FloatLiteral&>(*expr));
        break;
    case K::Ident:
        emitIdentifierExpr(static_cast<const wvmcc::parser::IdentifierExpr&>(*expr), needLValue);
        break;
    case K::Binary:
        emitBinaryExpr(static_cast<const wvmcc::parser::BinaryExpr&>(*expr));
        break;
    case K::Unary:
        emitUnaryExpr(static_cast<const wvmcc::parser::UnaryExpr&>(*expr), needLValue);
        break;
    case K::PostfixUnary:
        emitPostfixUnaryExpr(static_cast<const wvmcc::parser::PostfixUnaryExpr&>(*expr), needLValue);
        break;
    case K::Cast:
        emitCastExpr(static_cast<const wvmcc::parser::CastExpr&>(*expr));
        break;
    case K::String:
        emitStringLiteral(static_cast<const wvmcc::parser::StringLiteral&>(*expr));
        break;
    case K::Call:
        emitCallExpr(static_cast<const wvmcc::parser::CallExpr&>(*expr));
        break;
    case K::Member:
        emitMemberAccessExpr(static_cast<const wvmcc::parser::MemberExpr&>(*expr), needLValue);
        break;
    case K::Index:
        emitArrayIndexExpr(static_cast<const wvmcc::parser::IndexExpr&>(*expr), needLValue);
        break;
    case K::CompoundLiteral:
        emitCompoundLiteralExpr(static_cast<const wvmcc::parser::CompoundLiteral&>(*expr));
        break;
    case K::Ternary: {
        const auto& t = static_cast<const wvmcc::parser::TernaryExpr&>(*expr);
        if (!moduleCg_) { emitUnimplemented("codegen: ternary expression requires module context", expr->span); break; }

        // Result type: common arithmetic of the two branches. Use
        // typed if-else returning that type.
        auto thenVt = getExprType(t.thenExpr);
        auto elseVt = getExprType(t.elseExpr);
        // Pick the wider type.
        auto common = (thenVt == WasmVM::ValueType::f64 || elseVt == WasmVM::ValueType::f64) ? WasmVM::ValueType::f64
                    : (thenVt == WasmVM::ValueType::f32 || elseVt == WasmVM::ValueType::f32) ? WasmVM::ValueType::f32
                    : (thenVt == WasmVM::ValueType::i64 || elseVt == WasmVM::ValueType::i64) ? WasmVM::ValueType::i64
                    :                                                                          WasmVM::ValueType::i32;

        WasmVM::FuncType ifTy;
        ifTy.results.push_back(common);
        WasmVM::index_t ifTyIdx = moduleCg_->internFuncType(ifTy);

        // Emit cond, normalize to i32, then typed if.
        emitExpr(t.cond, false);
        auto condVt = getExprType(t.cond);
        if (condVt == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_const{0});
            emit(WasmVM::Instr::I64_ne{});
        } else if (condVt == WasmVM::ValueType::f32) {
            emit(WasmVM::Instr::F32_const{0.0f});
            emit(WasmVM::Instr::F32_ne{});
        } else if (condVt == WasmVM::ValueType::f64) {
            emit(WasmVM::Instr::F64_const{0.0});
            emit(WasmVM::Instr::F64_ne{});
        }
        emit(WasmVM::Instr::If{ifTyIdx});
        emitExpr(t.thenExpr, false);
        emitConvert(this, thenVt, common);
        emit(WasmVM::Instr::Else{});
        emitExpr(t.elseExpr, false);
        emitConvert(this, elseVt, common);
        emit(WasmVM::Instr::End{});
        break;
    }
    case K::Sizeof: {
        // `sizeof(type-name)` or `sizeof expr` → compile-time byte size as
        // size_t (i64 on wasm64). The operand expression is NOT evaluated
        // (C 6.5.3.4). For the type-name form, resolve the parsed specifiers
        // through Semantic so struct tags / typedef-names complete to their
        // definition (the parser-built `type` field is only a placeholder).
        const auto& so = static_cast<const wvmcc::parser::SizeofExpr&>(*expr);
        wvmcc::parser::TypeNodePtr opType;
        if (so.typeSpecs.has_value() && semantic_) {
            opType = semantic_->canonicalTypeRepr(*so.typeSpecs, nullptr);
        } else if (so.type.has_value()) {
            opType = *so.type;
        } else {
            opType = getExprTypeNode(so.expr);
        }
        if (!opType) {
            emitUnimplemented("codegen: cannot determine operand type of sizeof", expr->span);
            break;
        }
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)typeMap_.byteSize(opType)});
        break;
    }
    case K::AlignOf: {
        // `_Alignof(type-name)` → compile-time alignment as size_t (i64).
        const auto& ao = static_cast<const wvmcc::parser::AlignOfExpr&>(*expr);
        wvmcc::parser::TypeNodePtr opType;
        if (ao.typeSpecs.has_value() && semantic_) {
            opType = semantic_->canonicalTypeRepr(*ao.typeSpecs, nullptr);
        } else {
            opType = ao.type;
        }
        if (!opType) {
            emitUnimplemented("codegen: _Alignof missing type", expr->span);
            break;
        }
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)typeMap_.byteAlignment(opType)});
        break;
    }
    case K::GenericSelection: {
        // C 6.5.1.1: a generic selection lowers to its selected association; the
        // controlling expression is NOT evaluated.
        const auto& g = static_cast<const wvmcc::parser::GenericSelectionExpr&>(*expr);
        auto chosen = selectGenericAssociation(g);
        if (!chosen) {
            emitUnimplemented("codegen: no _Generic association matches the controlling type", expr->span);
            break;
        }
        emitExpr(chosen, needLValue);
        break;
    }
    default:
        emitUnimplemented("codegen not implemented: expression kind " + std::to_string((int)expr->kind), expr->span);
        break;
    }
}

void FunctionCodegen::emitIntegerLiteral(const wvmcc::parser::IntegerLiteral& expr) {
    // Honor explicit L/LL suffix in the source — `0ULL`, `1L` etc. must
    // emit as i64.const even though the *value* fits in i32. Otherwise
    // mixed-width comparisons (`acc > ULONG_MAX`) fail validation.
    bool forceLong = false;
    for (char c : expr.raw) {
        if (c == 'l' || c == 'L') { forceLong = true; break; }
    }
    // The literal's type (6.4.4.1) decides its wasm width: a value that fits the
    // 32-bit form of its signedness is `int`/`unsigned int` (i32), otherwise it
    // is a 64-bit type (i64). For unsigned literals the bound is UINT32_MAX, not
    // INT32_MAX — so `4294967295u` is a 32-bit `unsigned int`, not an i64.
    bool fitsI32 = expr.isUnsigned
                       ? (std::uint64_t)expr.value <= 0xFFFFFFFFULL
                       : (expr.value >= std::numeric_limits<int32_t>::min()
                          && expr.value <= std::numeric_limits<int32_t>::max());
    if (!forceLong && fitsI32) {
        emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)expr.value});
    } else {
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)expr.value});
    }
}

void FunctionCodegen::emitCharLiteral(const wvmcc::parser::CharLiteral& expr) {
    emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)expr.value});
}

void FunctionCodegen::emitFloatLiteral(const wvmcc::parser::FloatLiteral& expr) {
    if (expr.isFloat) {
        emit(WasmVM::Instr::F32_const{(WasmVM::f32_t)expr.value});
    } else {
        emit(WasmVM::Instr::F64_const{(WasmVM::f64_t)expr.value});
    }
}

void FunctionCodegen::emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr, bool needLValue) {
    auto symbolInfo = symbolTable_.lookup(expr.name);
    if (!symbolInfo) {
        // C 6.4.2.2: __func__ behaves as a static char array holding the
        // enclosing function's name. Emit a pointer to an interned string.
        if (!needLValue && expr.name == "__func__" && dataAllocator_) {
            size_t addr = dataAllocator_->internString(currentFunctionName_);
            dataPtrSites_.push_back({instrBuffer_.size(), addr});
            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)addr});
            return;
        }
        // #79: a bare function name in value context decays to a
        // function-pointer value (tagged i64 funcref-table slot).
        if (!needLValue) {
            auto funcSym = symbolTable_.lookupFunction(expr.name);
            if (funcSym) {
                emitFuncPtrValue(expr.name);
                return;
            }
        }
        // An enumeration constant referenced in a runtime expression (6.2.1/
        // 6.7.2.2): the parser leaves these as identifiers inside a function
        // body because a local could shadow the name. Symbol lookup just failed,
        // so no local/global/function shadows it — fold to its int value. (An
        // enum constant is never an lvalue, so this only applies in value
        // context.)
        if (!needLValue && moduleCg_) {
            if (auto ev = moduleCg_->lookupEnumConstant(expr.name)) {
                emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)*ev});
                return;
            }
        }
        emitUnimplemented("use of undeclared identifier '" + expr.name + "'", expr.span);
        return;
    }

    std::visit([this, needLValue](const auto& info) {
        using T = std::decay_t<decltype(info)>;
        if constexpr (std::is_same_v<T, ScalarLocal>) {
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)info.localIndex});
        } else if constexpr (std::is_same_v<T, MemoryLocal>) {
            // Compute shadow-stack address: fp + frameOffset (untagged mem[1]
            // offset). Direct named access (needLValue) consumes this with a
            // static mem[1] load/store.
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.frameOffset});
            emit(WasmVM::Instr::I64_add{});
            // Arrays decay to a pointer *value* in expression context: leave the
            // base address on the stack AND tag it with the mem[1] nibble so it
            // can be dereferenced through an opaque pointer elsewhere.
            bool isArray = info.type
                && info.type->kind == wvmcc::parser::TypeNode::Kind::Array;
            if (!needLValue && !isArray) {
                emit(typeMap_.makeLoad(info.type, 1));
            } else if (!needLValue && isArray) {
                emitApplyTag(AddrKind::Mem1);
            }
        } else if constexpr (std::is_same_v<T, GlobalScalar>) {
            emit(WasmVM::Instr::Global_get{(WasmVM::index_t)info.globalIndex});
        } else if constexpr (std::is_same_v<T, GlobalMem>) {
            // Static local / file-scope variable: address is in mem[0]
            // (baked const, or global.get for a cross-TU extern import).
            emitGlobalMemAddr(info);
            // Aggregates (arrays/structs/unions) decay to their address in
            // expression context — don't load a scalar out of them.
            bool isAggregate = info.type
                && (info.type->kind == wvmcc::parser::TypeNode::Kind::Array
                    || info.type->kind == wvmcc::parser::TypeNode::Kind::Struct
                    || info.type->kind == wvmcc::parser::TypeNode::Kind::Union);
            if (!needLValue && !isAggregate) {
                emit(typeMap_.makeLoad(info.type, 0));
            }
        } else {
            emitUnimplemented("codegen: unsupported symbol kind for identifier");
        }
    }, *symbolInfo);
}

// Build a synthetic integer-literal expression node (used to desugar
// ++/-- into `x = x + 1`). value=1 / raw="1" types as `int` (i32), which
// the binary-op and pointer-arithmetic paths promote as needed.
static wvmcc::parser::ExprPtr makeIntLiteralExpr(std::int64_t v) {
    auto il = wvmcc::parser::make_ast<wvmcc::parser::IntegerLiteral>();
    il->kind = wvmcc::parser::Expr::Kind::Integer;
    il->value = v;
    il->raw = std::to_string(v);
    return il;
}

// Build `lhs <op> rhs` as a BinaryExpr node (op is a plain binary operator
// like "+", "<<"). Shares the operand subtrees by pointer — safe because
// codegen only reads them.
static wvmcc::parser::ExprPtr makeBinaryExpr(const std::string& op,
                                             const wvmcc::parser::ExprPtr& lhs,
                                             const wvmcc::parser::ExprPtr& rhs,
                                             const wvmcc::SourceSpan& span) {
    auto be = wvmcc::parser::make_ast<wvmcc::parser::BinaryExpr>();
    be->kind = wvmcc::parser::Expr::Kind::Binary;
    be->op = op;
    be->lhs = lhs;
    be->rhs = rhs;
    be->span = span;
    return be;
}

void FunctionCodegen::emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr) {
    using K = wvmcc::parser::Expr::Kind;

    // --- Compound assignment: `lhs OP= rhs`  ==>  `lhs = (lhs OP rhs)` ---
    // Reuses the plain-`=` lvalue machinery and the binary-op (incl. pointer-
    // arithmetic) machinery below. The lhs subtree is evaluated twice; for the
    // address-bearing lvalues libc uses (locals, `*p`, `a[i]`, `s->m`) that is
    // observationally fine. Yields the assigned value, as C requires.
    {
        static const std::unordered_map<std::string, std::string> kCompound = {
            {"+=", "+"}, {"-=", "-"}, {"*=", "*"}, {"/=", "/"}, {"%=", "%"},
            {"<<=", "<<"}, {">>=", ">>"}, {"&=", "&"}, {"|=", "|"}, {"^=", "^"}};
        auto it = kCompound.find(expr.op);
        if (it != kCompound.end()) {
            auto inner = makeBinaryExpr(it->second, expr.lhs, expr.rhs, expr.span);
            auto assign = makeBinaryExpr("=", expr.lhs, inner, expr.span);
            emitBinaryExpr(static_cast<const wvmcc::parser::BinaryExpr&>(*assign));
            return;
        }
    }

    // --- Comma operator (6.5.17): evaluate the left operand as a void
    // expression (discarding its value), then yield the right operand. ---
    if (expr.op == ",") {
        emitExpr(expr.lhs, false);
        if (exprLeavesValue(expr.lhs)) emit(WasmVM::Instr::Drop{});
        emitExpr(expr.rhs, false);
        return;
    }

    // --- Assignment ---
    if (expr.op == "=") {
        // ScalarLocal: value → local.tee (sets local, leaves value on stack)
        if (expr.lhs && expr.lhs->kind == K::Ident) {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.lhs);
            auto sym = symbolTable_.lookup(id.name);
            if (sym) {
                if (auto* sl = std::get_if<ScalarLocal>(&*sym)) {
                    emitExpr(expr.rhs, false);
                    // Convert rhs to lhs type if they differ (e.g. `long x = 1`
                    // promotes the i32 literal to i64 before the local.tee).
                    auto rhsVt = getExprType(expr.rhs);
                    auto lhsVt = sl->type ? typeMap_.toWasmType(sl->type)
                                          : WasmVM::ValueType::i32;
                    emitConvert(this, rhsVt, lhsVt);
                    // _Bool: normalize the assigned value to 0/1.
                    if (sl->type
                        && sl->type->kind == wvmcc::parser::TypeNode::Kind::Builtin
                        && !sl->type->simple.empty()
                        && sl->type->simple[0]
                           == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Bool) {
                        emit(WasmVM::Instr::I32_const{0});
                        emit(WasmVM::Instr::I32_ne{});
                    }
                    // Assignment converts the value to the lvalue's type
                    // (6.5.16.1); for a char/short local that means truncating
                    // to its width (raw i32 locals carry no width on their own).
                    emitIntegerNarrow(sl->type);
                    emit(WasmVM::Instr::Local_tee{(WasmVM::index_t)sl->localIndex});
                    return;
                }
                if (auto* ml = std::get_if<MemoryLocal>(&*sym)) {
                    auto rhsWasmType = getExprType(expr.rhs);
                    int tempIdx = allocRawLocal(rhsWasmType);
                    emitExpr(expr.rhs, false);
                    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)tempIdx});
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
                    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)ml->frameOffset});
                    emit(WasmVM::Instr::I64_add{});
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
                    emit(typeMap_.makeStore(ml->type, 1));
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
                    return;
                }
                if (auto* gm = std::get_if<GlobalMem>(&*sym)) {
                    // Static local / file-scope variable: address is in mem[0].
                    auto rhsWasmType = getExprType(expr.rhs);
                    int tempIdx = allocRawLocal(rhsWasmType);
                    emitExpr(expr.rhs, false);
                    auto gmVt = gm->type ? typeMap_.toWasmType(gm->type)
                                         : WasmVM::ValueType::i32;
                    emitConvert(this, rhsWasmType, gmVt);
                    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)tempIdx});
                    emitGlobalMemAddr(*gm);
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
                    emit(typeMap_.makeStore(gm->type, 0));
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
                    return;
                }
                if (auto* gs = std::get_if<GlobalScalar>(&*sym)) {
                    // File-scope/runtime Wasm global (e.g. __stack_pointer).
                    auto rhsWasmType = getExprType(expr.rhs);
                    auto gsVt = gs->type ? typeMap_.toWasmType(gs->type)
                                         : WasmVM::ValueType::i64;
                    emitExpr(expr.rhs, false);
                    emitConvert(this, rhsWasmType, gsVt);
                    // Leave the value on the stack as the assignment result:
                    // tee into the global via a temp.
                    int tempIdx = allocRawLocal(gsVt);
                    emit(WasmVM::Instr::Local_tee{(WasmVM::index_t)tempIdx});
                    emit(WasmVM::Instr::Global_set{(WasmVM::index_t)gs->globalIndex});
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
                    return;
                }
            }
        }

        // General lvalue assignment (pointer dereference, member access, array index)
        // Convert the rhs to the lvalue's type up front so the store sees the
        // right Wasm type (e.g. storing the literal 0 — i32 — into an i64
        // pointer/size_t field) and the assignment's value has the lvalue type.
        auto lhsTypeNode = getExprTypeNode(expr.lhs);
        auto rhsWasmType = getExprType(expr.rhs);
        auto lhsWasmType = lhsTypeNode ? typeMap_.toWasmType(lhsTypeNode) : rhsWasmType;
        int tempIdx = allocRawLocal(lhsWasmType);
        emitExpr(expr.rhs, false);
        emitConvert(this, rhsWasmType, lhsWasmType);
        emit(WasmVM::Instr::Local_set{(WasmVM::index_t)tempIdx});

        emitExpr(expr.lhs, true);  // push lhs address
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});

        // A named-object lvalue resolves to a static memory and an untagged
        // frame/static address; a pointer-rooted lvalue is Dynamic and its
        // address carries the memidx tag, so dispatch on it.
        AddrKind k = expr.lhs ? addressKind(expr.lhs.get()) : AddrKind::Mem1;
        if (k == AddrKind::Dynamic) {
            emitTaggedStore(lhsTypeNode);
        } else {
            emit(typeMap_.makeStore(lhsTypeNode, k == AddrKind::Mem1 ? 1 : 0));
        }
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
        return;
    }

    // --- Short-circuit logical operators ---
    // `lhs && rhs`: if lhs == 0 → 0; else (rhs != 0).
    // `lhs || rhs`: if lhs != 0 → 1; else (rhs != 0).
    // Lowered via a typed `if (block-type result i32)` over the lhs result.
    if ((expr.op == "&&" || expr.op == "||") && moduleCg_) {
        const bool isAnd = (expr.op == "&&");

        // Intern the block type `() -> i32` so the `if` can produce a value.
        WasmVM::FuncType ifTy;
        ifTy.results.push_back(WasmVM::ValueType::i32);
        WasmVM::index_t ifTyIdx = moduleCg_->internFuncType(ifTy);

        // Normalize lhs to i32 0/1 before the branch.
        emitExpr(expr.lhs, false);
        auto lhsVt = getExprType(expr.lhs);
        if (lhsVt == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_const{0});
            emit(WasmVM::Instr::I64_ne{});
        } else if (lhsVt == WasmVM::ValueType::f32) {
            emit(WasmVM::Instr::F32_const{0.0f});
            emit(WasmVM::Instr::F32_ne{});
        } else if (lhsVt == WasmVM::ValueType::f64) {
            emit(WasmVM::Instr::F64_const{0.0});
            emit(WasmVM::Instr::F64_ne{});
        } else {
            // i32 already; squash to 0/1.
            emit(WasmVM::Instr::I32_const{0});
            emit(WasmVM::Instr::I32_ne{});
        }

        emit(WasmVM::Instr::If{ifTyIdx});
        if (isAnd) {
            // lhs true → evaluate rhs, normalize.
            emitExpr(expr.rhs, false);
            auto rhsVt = getExprType(expr.rhs);
            if (rhsVt == WasmVM::ValueType::i64) {
                emit(WasmVM::Instr::I64_const{0});
                emit(WasmVM::Instr::I64_ne{});
            } else if (rhsVt == WasmVM::ValueType::f32) {
                emit(WasmVM::Instr::F32_const{0.0f});
                emit(WasmVM::Instr::F32_ne{});
            } else if (rhsVt == WasmVM::ValueType::f64) {
                emit(WasmVM::Instr::F64_const{0.0});
                emit(WasmVM::Instr::F64_ne{});
            } else {
                emit(WasmVM::Instr::I32_const{0});
                emit(WasmVM::Instr::I32_ne{});
            }
        } else {
            // lhs true → result is 1 (short circuit).
            emit(WasmVM::Instr::I32_const{1});
        }
        emit(WasmVM::Instr::Else{});
        if (isAnd) {
            // lhs false → result is 0.
            emit(WasmVM::Instr::I32_const{0});
        } else {
            // lhs false → evaluate rhs, normalize.
            emitExpr(expr.rhs, false);
            auto rhsVt = getExprType(expr.rhs);
            if (rhsVt == WasmVM::ValueType::i64) {
                emit(WasmVM::Instr::I64_const{0});
                emit(WasmVM::Instr::I64_ne{});
            } else if (rhsVt == WasmVM::ValueType::f32) {
                emit(WasmVM::Instr::F32_const{0.0f});
                emit(WasmVM::Instr::F32_ne{});
            } else if (rhsVt == WasmVM::ValueType::f64) {
                emit(WasmVM::Instr::F64_const{0.0});
                emit(WasmVM::Instr::F64_ne{});
            } else {
                emit(WasmVM::Instr::I32_const{0});
                emit(WasmVM::Instr::I32_ne{});
            }
        }
        emit(WasmVM::Instr::End{});
        return;
    }

    // --- Pointer arithmetic (+/-) ---
    // A pointer or an array (which decays to a pointer to its element) may be
    // an operand. `ptr + int` and `int + ptr` are both valid; the integer side
    // is scaled by the pointee size. (`ptr - ptr` is a different case left to
    // the default path.)
    auto lhsTypeNode = getExprTypeNode(expr.lhs);
    auto rhsTypeNode = getExprTypeNode(expr.rhs);
    auto pointeeType = [](const wvmcc::parser::TypeNodePtr& t) -> wvmcc::parser::TypeNodePtr {
        if (!t) return nullptr;
        if (t->kind == wvmcc::parser::TypeNode::Kind::Pointer) return t->pointee;
        if (t->kind == wvmcc::parser::TypeNode::Kind::Array)   return t->element;
        return nullptr;
    };
    bool lhsPtrLike = lhsTypeNode && (lhsTypeNode->kind == wvmcc::parser::TypeNode::Kind::Pointer
                                      || lhsTypeNode->kind == wvmcc::parser::TypeNode::Kind::Array);
    bool rhsPtrLike = rhsTypeNode && (rhsTypeNode->kind == wvmcc::parser::TypeNode::Kind::Pointer
                                      || rhsTypeNode->kind == wvmcc::parser::TypeNode::Kind::Array);

    if ((expr.op == "+" || expr.op == "-") && (lhsPtrLike != rhsPtrLike)) {
        const auto& ptrExpr  = lhsPtrLike ? expr.lhs : expr.rhs;
        const auto& intExpr  = lhsPtrLike ? expr.rhs : expr.lhs;
        auto ptrType         = lhsPtrLike ? lhsTypeNode : rhsTypeNode;
        size_t pointeeSize = 1;
        if (auto pt = pointeeType(ptrType)) { size_t s = typeMap_.byteSize(pt); if (s) pointeeSize = s; }
        emitExpr(ptrExpr, false);                       // base address (i64)
        emitExpr(intExpr, false);                       // index
        if (getExprType(intExpr) == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I64_extend_i32_s{});
        }
        if (pointeeSize > 1) {
            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)pointeeSize});
            emit(WasmVM::Instr::I64_mul{});
        }
        if (expr.op == "+") emit(WasmVM::Instr::I64_add{});
        else                 emit(WasmVM::Instr::I64_sub{});
        return;
    }

    // --- Normal binary ops ---
    // Compute the common arithmetic type (usual arithmetic conversions) and
    // promote both operands to it before dispatching. Float operands route to
    // F32/F64 instructions; bitwise/shift ops stay int-only.
    auto lhsValueType = getExprType(expr.lhs);
    auto rhsValueType = getExprType(expr.rhs);
    bool isBitwise = (expr.op == "&" || expr.op == "|" || expr.op == "^"
                      || expr.op == "<<" || expr.op == ">>");
    auto commonType = isBitwise
        ? (lhsValueType == WasmVM::ValueType::i64 || rhsValueType == WasmVM::ValueType::i64
              ? WasmVM::ValueType::i64
              : WasmVM::ValueType::i32)
        : arithCommonType(lhsValueType, rhsValueType);

    // Signedness for the operations that differ by it (division, remainder,
    // ordered comparison, right shift). Per the usual arithmetic conversions
    // (6.3.1.8), an operand pair is treated as unsigned when either operand is
    // unsigned; a right shift looks only at the (left) value being shifted.
    bool lhsUnsigned = typeMap_.isUnsignedScalarInteger(getExprTypeNode(expr.lhs));
    bool rhsUnsigned = typeMap_.isUnsignedScalarInteger(getExprTypeNode(expr.rhs));
    bool opUnsigned = lhsUnsigned || rhsUnsigned;
    // Pick the unsigned or signed form of an instruction by a signedness flag.
    auto emitSU = [&](bool uns, auto unsignedInstr, auto signedInstr) {
        if (uns) emit(unsignedInstr); else emit(signedInstr);
    };

    emitExpr(expr.lhs, false);
    emitConvert(this, lhsValueType, commonType);
    emitExpr(expr.rhs, false);
    emitConvert(this, rhsValueType, commonType);

    using VT = WasmVM::ValueType;
    if (expr.op == "+") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_add{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_add{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_add{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_add{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "-") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_sub{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_sub{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_sub{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_sub{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "*") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_mul{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_mul{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_mul{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_mul{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "/") {
        if (commonType == VT::i32) emitSU(opUnsigned, WasmVM::Instr::I32_div_u{}, WasmVM::Instr::I32_div_s{});
        else if (commonType == VT::i64) emitSU(opUnsigned, WasmVM::Instr::I64_div_u{}, WasmVM::Instr::I64_div_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_div{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_div{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "%") {
        // C: % is integer-only; floats are a constraint violation.
        if (commonType == VT::i32) emitSU(opUnsigned, WasmVM::Instr::I32_rem_u{}, WasmVM::Instr::I32_rem_s{});
        else if (commonType == VT::i64) emitSU(opUnsigned, WasmVM::Instr::I64_rem_u{}, WasmVM::Instr::I64_rem_s{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "==") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_eq{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_eq{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_eq{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_eq{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "!=") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_ne{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_ne{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_ne{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_ne{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "<") {
        if (commonType == VT::i32) emitSU(opUnsigned, WasmVM::Instr::I32_lt_u{}, WasmVM::Instr::I32_lt_s{});
        else if (commonType == VT::i64) emitSU(opUnsigned, WasmVM::Instr::I64_lt_u{}, WasmVM::Instr::I64_lt_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_lt{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_lt{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == ">") {
        if (commonType == VT::i32) emitSU(opUnsigned, WasmVM::Instr::I32_gt_u{}, WasmVM::Instr::I32_gt_s{});
        else if (commonType == VT::i64) emitSU(opUnsigned, WasmVM::Instr::I64_gt_u{}, WasmVM::Instr::I64_gt_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_gt{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_gt{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "<=") {
        if (commonType == VT::i32) emitSU(opUnsigned, WasmVM::Instr::I32_le_u{}, WasmVM::Instr::I32_le_s{});
        else if (commonType == VT::i64) emitSU(opUnsigned, WasmVM::Instr::I64_le_u{}, WasmVM::Instr::I64_le_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_le{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_le{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == ">=") {
        if (commonType == VT::i32) emitSU(opUnsigned, WasmVM::Instr::I32_ge_u{}, WasmVM::Instr::I32_ge_s{});
        else if (commonType == VT::i64) emitSU(opUnsigned, WasmVM::Instr::I64_ge_u{}, WasmVM::Instr::I64_ge_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_ge{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_ge{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "&") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_and{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_and{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "|") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_or{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_or{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "^") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_xor{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_xor{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == "<<") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_shl{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_shl{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else if (expr.op == ">>") {
        // Right shift uses the signedness of the value being shifted (the left
        // operand): logical for unsigned, arithmetic for signed.
        if (commonType == VT::i32) emitSU(lhsUnsigned, WasmVM::Instr::I32_shr_u{}, WasmVM::Instr::I32_shr_s{});
        else if (commonType == VT::i64) emitSU(lhsUnsigned, WasmVM::Instr::I64_shr_u{}, WasmVM::Instr::I64_shr_s{});
        else emitUnimplemented("codegen: unsupported operand type for binary operator '" + expr.op + "'", expr.span);
    } else {
        emitUnimplemented("codegen not implemented: binary operator '" + expr.op + "'", expr.span);
    }
}

void FunctionCodegen::emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr, bool needLValue) {
    // Address-of: emit the inner expression as an lvalue (leaves i64 address on stack)
    if (expr.op == "&") {
        // #79: &funcname produces a function-pointer value (tagged i64
        // funcref-table slot), identical to the bare-name decay.
        if (expr.rhs && expr.rhs->kind == wvmcc::parser::Expr::Kind::Ident) {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.rhs);
            auto funcSym = symbolTable_.lookupFunction(id.name);
            if (funcSym) {
                emitFuncPtrValue(id.name);
                return;
            }
        }
        // &lvalue yields a pointer *value*: take the (untagged) lvalue address
        // and tag it with the object's memidx. For a pointer-rooted lvalue
        // (&*p, &p->m, &p[i]) the address already carries p's tag and the kind
        // is Dynamic, so emitApplyTag is a no-op.
        emitExpr(expr.rhs, true);
        emitApplyTag(addressKind(expr.rhs.get()));
        return;
    }

    // Dereference: emit the pointer value, then load the pointee. The pointer
    // carries its memidx in the high nibble, so dispatch on the tag.
    if (expr.op == "*") {
        emitExpr(expr.rhs, false);
        if (!needLValue) {
            auto rhsTypeNode = getExprTypeNode(expr.rhs);
            wvmcc::parser::TypeNodePtr pointeeType;
            if (rhsTypeNode && rhsTypeNode->kind == wvmcc::parser::TypeNode::Kind::Pointer)
                pointeeType = rhsTypeNode->pointee;
            emitTaggedLoad(pointeeType);
        }
        return;
    }

    // Prefix ++/-- : `++x` ==> `x = x + 1`, `--x` ==> `x = x - 1`. Yields the
    // new value (the `=` path leaves the assigned value on the stack).
    if (expr.op == "++" || expr.op == "--") {
        auto add = makeBinaryExpr(expr.op == "++" ? "+" : "-",
                                  expr.rhs, makeIntLiteralExpr(1), expr.span);
        auto assign = makeBinaryExpr("=", expr.rhs, add, expr.span);
        emitBinaryExpr(static_cast<const wvmcc::parser::BinaryExpr&>(*assign));
        return;
    }

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
            emitUnimplemented("codegen: unsupported operand type for unary '-'", expr.span);
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
            emitUnimplemented("codegen: unsupported operand type for unary '~'", expr.span);
        }
    } else if (expr.op == "!") {
        if (exprType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_eqz{});
        } else if (exprType == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I64_eqz{});
        } else {
            emitUnimplemented("codegen: unsupported operand type for unary '!'", expr.span);
        }
    } else if (expr.op == "+") {
        // no-op
    } else {
        emitUnimplemented("codegen not implemented: unary operator '" + expr.op + "'", expr.span);
    }
}

void FunctionCodegen::emitPostfixUnaryExpr(const wvmcc::parser::PostfixUnaryExpr& expr,
                                           bool /*needLValue*/) {
    // `x++` ==> evaluate old value into a temp, then `x = x + 1`, then yield
    // the old value. The base subtree is evaluated twice (once for the old
    // value, once inside the desugared assignment); fine for the lvalue forms
    // libc uses. Result is always an rvalue.
    using PUOp = wvmcc::parser::PostfixUnaryExpr::Op;
    auto baseVt = getExprType(expr.base);

    emitExpr(expr.base, false);                 // old value on stack
    int tmp = allocRawLocal(baseVt);
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)tmp});

    auto add = makeBinaryExpr(expr.op == PUOp::Inc ? "+" : "-",
                              expr.base, makeIntLiteralExpr(1), expr.span);
    auto assign = makeBinaryExpr("=", expr.base, add, expr.span);
    emitBinaryExpr(static_cast<const wvmcc::parser::BinaryExpr&>(*assign));
    emit(WasmVM::Instr::Drop{});                // discard the new value
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tmp}); // push old value
}

// True when `type` (qualifiers stripped) is the `_Bool` builtin.
static bool isBoolTypeNode(const wvmcc::parser::TypeNodePtr& type) {
    using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
    auto t = type;
    while (t && t->kind == wvmcc::parser::TypeNode::Kind::Qualified) t = t->pointee;
    if (!t || t->kind != wvmcc::parser::TypeNode::Kind::Builtin) return false;
    for (auto s : t->simple) if (s == STS::Bool) return true;
    return false;
}

void FunctionCodegen::emitCastExpr(const wvmcc::parser::CastExpr& expr) {
    emitExpr(expr.expr, false);

    auto targetType = typeMap_.toWasmType(expr.type);
    auto sourceType = getExprType(expr.expr);

    // Cast to _Bool (6.3.1.2): the result is 0 if the operand compares equal to
    // 0, and 1 otherwise — a normalization, not a truncation. Test on the source
    // representation (before any lossy wrap) so high bits of a wide operand
    // still count.
    if (isBoolTypeNode(expr.type)) {
        switch (sourceType) {
            case WasmVM::ValueType::i64: emit(WasmVM::Instr::I64_eqz{}); emit(WasmVM::Instr::I32_eqz{}); break;
            case WasmVM::ValueType::f32: emit(WasmVM::Instr::F32_const{0}); emit(WasmVM::Instr::F32_ne{}); break;
            case WasmVM::ValueType::f64: emit(WasmVM::Instr::F64_const{0}); emit(WasmVM::Instr::F64_ne{}); break;
            default:                     emit(WasmVM::Instr::I32_eqz{}); emit(WasmVM::Instr::I32_eqz{}); break;
        }
        return;
    }

    if (sourceType == targetType) {
        // no-op (width narrowing below still applies for char/short targets)
    } else if (sourceType == WasmVM::ValueType::i32 && targetType == WasmVM::ValueType::i64) {
        emit(WasmVM::Instr::I64_extend_i32_s{});
    } else if (sourceType == WasmVM::ValueType::i64 && targetType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I32_wrap_i64{});
    } else if (sourceType == WasmVM::ValueType::f32 && targetType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I32_trunc_sat_f32_s{});
    } else if (sourceType == WasmVM::ValueType::f64 && targetType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I32_trunc_sat_f64_s{});
    } else if (sourceType == WasmVM::ValueType::f32 && targetType == WasmVM::ValueType::i64) {
        emit(WasmVM::Instr::I64_trunc_sat_f32_s{});
    } else if (sourceType == WasmVM::ValueType::f64 && targetType == WasmVM::ValueType::i64) {
        emit(WasmVM::Instr::I64_trunc_sat_f64_s{});
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
        emitUnimplemented("codegen: unsupported cast conversion", expr.span);
    }

    // Conversion to a narrow integer type (6.3.1.3): char/short are represented
    // as i32, so the value must additionally be reduced to the target width.
    emitIntegerNarrow(expr.type);
}

void FunctionCodegen::emitIntegerNarrow(const wvmcc::parser::TypeNodePtr& targetType) {
    if (!targetType) return;
    if (typeMap_.toWasmType(targetType) != WasmVM::ValueType::i32) return; // only i32-width ints
    if (isBoolTypeNode(targetType)) return; // _Bool normalizes to 0/1 elsewhere
    size_t width = typeMap_.byteSize(targetType);
    bool isUnsigned = typeMap_.isUnsignedScalarInteger(targetType);
    if (width == 1) {
        if (isUnsigned) { emit(WasmVM::Instr::I32_const{0xFF}); emit(WasmVM::Instr::I32_and{}); }
        else { emit(WasmVM::Instr::I32_const{24}); emit(WasmVM::Instr::I32_shl{}); emit(WasmVM::Instr::I32_const{24}); emit(WasmVM::Instr::I32_shr_s{}); }
    } else if (width == 2) {
        if (isUnsigned) { emit(WasmVM::Instr::I32_const{0xFFFF}); emit(WasmVM::Instr::I32_and{}); }
        else { emit(WasmVM::Instr::I32_const{16}); emit(WasmVM::Instr::I32_shl{}); emit(WasmVM::Instr::I32_const{16}); emit(WasmVM::Instr::I32_shr_s{}); }
    }
}

void FunctionCodegen::emitStringLiteral(const wvmcc::parser::StringLiteral& expr) {
    if (!dataAllocator_) {
        emitUnimplemented("codegen: string literal requires a data allocator", expr.span);
        return;
    }
    size_t addr = dataAllocator_->internString(expr.value);
    // Record the data-pointer site BEFORE the emit() so we capture the
    // instruction index of the i64.const itself (not the next instruction).
    dataPtrSites_.push_back({instrBuffer_.size(), addr});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)addr});
}

void FunctionCodegen::emitVaBuiltin(const std::string& name,
                                    const wvmcc::parser::CallExpr& expr) {
    // Helper: assign the i64 value currently on top of the Wasm stack into
    // a ScalarLocal va_list variable identified by `apExpr`. Falls back to
    // a diagnostic if `apExpr` is not a plain identifier referring to a
    // ScalarLocal — wvmcc's va_list ABI assumes that form.
    auto assignTopToApLocal = [&](const wvmcc::parser::ExprPtr& apExpr) {
        if (apExpr && apExpr->kind == wvmcc::parser::Expr::Kind::Ident) {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*apExpr);
            auto sym = symbolTable_.lookup(id.name);
            if (sym) {
                if (auto* sl = std::get_if<ScalarLocal>(&*sym)) {
                    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)sl->localIndex});
                    return;
                }
            }
        }
        wvmcc::Diagnostic d;
        d.severity = wvmcc::Diagnostic::Severity::Error;
        d.message = "va_list argument must be a plain local variable";
        diagnostics_.push_back(std::move(d));
        emit(WasmVM::Instr::Drop{});
    };

    if (name == "__builtin_va_start") {
        // ap = __va_args_ptr (the hidden trailing param of the enclosing callee)
        if (expr.args.empty()) return;
        if (vaArgsPtrLocal_ < 0) {
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "__builtin_va_start used outside a variadic function";
            diagnostics_.push_back(std::move(d));
            return;
        }
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)vaArgsPtrLocal_});
        assignTopToApLocal(expr.args[0]);
        return;
    }

    if (name == "__builtin_va_end") {
        return; // no-op
    }

    if (name == "__builtin_va_copy") {
        if (expr.args.size() < 2) return;
        emitExpr(expr.args[1]);
        assignTopToApLocal(expr.args[0]);
        return;
    }

    if (name == "__builtin_va_arg") {
        // Result type T comes from expr.vaArgType (parsed as a type-name).
        if (expr.args.empty()) return;
        auto vargType = expr.vaArgType;
        auto resultWasmType = vargType ? typeMap_.toWasmType(vargType)
                                       : WasmVM::ValueType::i32;

        // 1. Load *ap as i64 from mem[1]; coerce to T.
        emitExpr(expr.args[0]); // pointer value (i64) at ap
        emit(WasmVM::Instr::I64_load{1, 0, 3});
        if (resultWasmType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I32_wrap_i64{});
        } else if (resultWasmType == WasmVM::ValueType::f32) {
            // Slot holds a double (default promotion); reinterpret + demote.
            emit(WasmVM::Instr::F64_reinterpret_i64{});
            emit(WasmVM::Instr::F32_demote_f64{});
        } else if (resultWasmType == WasmVM::ValueType::f64) {
            emit(WasmVM::Instr::F64_reinterpret_i64{});
        }

        // 2. Stash the result in a temp, advance ap by 8, then push result back.
        int tmpLocal = allocRawLocal(resultWasmType);
        emit(WasmVM::Instr::Local_set{(WasmVM::index_t)tmpLocal});

        emitExpr(expr.args[0]); // current ap value
        emit(WasmVM::Instr::I64_const{8});
        emit(WasmVM::Instr::I64_add{});
        assignTopToApLocal(expr.args[0]);

        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tmpLocal});
        return;
    }
}

void FunctionCodegen::emitCallExpr(const wvmcc::parser::CallExpr& expr) {
    constexpr WasmVM::index_t kStackPtrIdx = 0;

    // Direct call iff the callee is a bare identifier referring to a known
    // function symbol. Otherwise this is an indirect (function-pointer) call
    // and we lower it via `call_indirect`.
    bool isDirect = false;
    std::string directName;
    if (expr.callee && expr.callee->kind == wvmcc::parser::Expr::Kind::Ident) {
        const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.callee);
        if (symbolTable_.lookupFunction(id.name).has_value()) {
            isDirect = true;
            directName = id.name;
        }
    }

    // __builtin_va_* are recognized by name and lowered inline. They are not
    // real Wasm functions; the symbol table never registers them.
    if (expr.callee && expr.callee->kind == wvmcc::parser::Expr::Kind::Ident) {
        const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.callee);
        if (id.name == "__builtin_va_start" || id.name == "__builtin_va_end"
            || id.name == "__builtin_va_copy" || id.name == "__builtin_va_arg") {
            emitVaBuiltin(id.name, expr);
            return;
        }
    }

    // Determine variadic-ness and named-parameter count of the callee.
    bool calleeIsVariadic = false;
    int namedParamCount = 0;
    if (isDirect) {
        auto funcSym = symbolTable_.lookupFunction(directName);
        if (funcSym) {
            calleeIsVariadic = funcSym->isVariadic;
            namedParamCount = funcSym->namedParamCount;
        }
    } else {
        auto calleeType = getExprTypeNode(expr.callee);
        auto fnNode = calleeType;
        if (fnNode && fnNode->kind == wvmcc::parser::TypeNode::Kind::Pointer) {
            fnNode = fnNode->pointee;
        }
        if (fnNode && fnNode->kind == wvmcc::parser::TypeNode::Kind::Function) {
            calleeIsVariadic = fnNode->isVariadic;
            namedParamCount = static_cast<int>(fnNode->params.size());
        }
    }

    // Check if callee returns a struct (needs hidden sret buffer).
    wvmcc::parser::TypeNodePtr calleeRetType;
    int sretBufLocal = -1;

    // A direct call to a `_Noreturn` function (exit, abort, …) never returns,
    // so the path after it is unreachable. Emitting a trailing `unreachable`
    // keeps a non-void caller valid when such a call is its last statement —
    // otherwise control falls through to the function `end` with nothing on the
    // operand stack, which the Wasm validator rejects. Indirect calls can't be
    // proven no-return, so they're left as ordinary calls.
    bool calleeNoReturn = false;

    if (isDirect) {
        auto funcSym = symbolTable_.lookupFunction(directName);
        if (funcSym && funcSym->type
            && (funcSym->type->kind == wvmcc::parser::TypeNode::Kind::Struct
                || funcSym->type->kind == wvmcc::parser::TypeNode::Kind::Union)) {
            calleeRetType = funcSym->type;
        }
        if (funcSym) calleeNoReturn = funcSym->noReturn;
    }

    if (calleeRetType) {
        // Allocate a shadow-stack buffer for the returned struct.
        if (framePointerLocal_ == -1) {
            framePointerLocal_ = allocRawLocal(WasmVM::ValueType::i64);
        }
        size_t align = typeMap_.byteAlignment(calleeRetType);
        size_t size  = typeMap_.byteSize(calleeRetType);
        if (align > 1) frameSize_ = (frameSize_ + align - 1) & ~(align - 1);
        size_t bufOff = frameSize_;
        frameSize_ += size;

        // Emit buffer address as hidden sret first argument.
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)bufOff});
        emit(WasmVM::Instr::I64_add{});
        sretBufLocal = allocRawLocal(WasmVM::ValueType::i64);
        emit(WasmVM::Instr::Local_tee{(WasmVM::index_t)sretBufLocal});
    }

    auto emitDirectCall = [&]() {
        auto funcSym = symbolTable_.lookupFunction(directName);
        emit(WasmVM::Instr::Call{(WasmVM::index_t)funcSym->funcIndex});
    };

    auto emitIndirectCall = [&]() {
        // #79: function pointers are tagged-i64 funcref-table slots. Push the
        // pointer value (the table index, after masking the tag) and use
        // call_indirect <table 0> <typeidx>.
        emitExpr(expr.callee, false);

        std::optional<WasmVM::index_t> typeIdx;
        auto calleeType = getExprTypeNode(expr.callee);
        auto fnNode = calleeType;
        if (fnNode && fnNode->kind == wvmcc::parser::TypeNode::Kind::Pointer) {
            fnNode = fnNode->pointee;
        }
        if (fnNode && fnNode->kind == wvmcc::parser::TypeNode::Kind::Function && moduleCg_) {
            WasmVM::FuncType ft;
            if (fnNode->element) {
                bool isVoid = fnNode->element->kind == wvmcc::parser::TypeNode::Kind::Builtin
                              && !fnNode->element->simple.empty()
                              && fnNode->element->simple[0]
                                 == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void;
                if (!isVoid) ft.results.push_back(typeMap_.toWasmType(fnNode->element));
            }
            // A lone `(void)` parameter list means zero parameters — don't
            // synthesize a spurious i32 param (which would mismatch the callee's
            // real type and underflow the operand stack on a no-arg call).
            auto isVoidParam = [](const wvmcc::parser::TypeNodePtr& p) {
                return p && p->kind == wvmcc::parser::TypeNode::Kind::Builtin
                    && p->simple.size() == 1
                    && p->simple[0]
                       == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void;
            };
            if (!(fnNode->params.size() == 1 && isVoidParam(fnNode->params[0]))) {
                for (const auto& p : fnNode->params) {
                    ft.params.push_back(typeMap_.toWasmType(p));
                }
            }
            if (fnNode->isVariadic) {
                ft.params.push_back(WasmVM::ValueType::i64); // trailing va_args ptr
            }
            typeIdx = moduleCg_->internFuncType(ft);
        }
        if (!typeIdx && expr.callee && expr.callee->kind == wvmcc::parser::Expr::Kind::Ident && moduleCg_) {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.callee);
            typeIdx = moduleCg_->getFuncTypeIdx(id.name);
        }

        if (!typeIdx) {
            emit(WasmVM::Instr::Unreachable{});
            wvmcc::Diagnostic d;
            d.severity = wvmcc::Diagnostic::Severity::Error;
            d.message = "indirect call: unable to determine callee type";
            diagnostics_.push_back(std::move(d));
            return;
        }

        // Mask off the function-pointer tag to recover the table slot, narrow
        // to i32, and dispatch through funcref table 0.
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)kPtrOffMask});
        emit(WasmVM::Instr::I64_and{});
        emit(WasmVM::Instr::I32_wrap_i64{});
        emit(WasmVM::Instr::Call_indirect{(WasmVM::index_t)0, *typeIdx});
    };

    if (!calleeIsVariadic) {
        // Non-variadic path: push args in order, call.
        // Coerce each argument to the parameter's Wasm value type so
        // passing e.g. an `int` literal (i32) to a `size_t` (i64)
        // parameter doesn't break validation. Only direct calls have a
        // resolved paramTypes vector available.
        std::vector<WasmVM::ValueType> paramTypes;
        if (isDirect) {
            auto funcSym = symbolTable_.lookupFunction(directName);
            if (funcSym) paramTypes = funcSym->paramTypes;
        }
        for (size_t i = 0; i < expr.args.size(); ++i) {
            emitExpr(expr.args[i]);
            if (i < paramTypes.size()) {
                // Convert each argument to the parameter type as by assignment
                // (6.5.2.2p7), including int<->float, so e.g. `take_int(2.9)` or
                // `take_double(3)` neither mistypes the operand stack nor passes
                // an unconverted value.
                emitConvert(this, getExprType(expr.args[i]), paramTypes[i]);
            }
        }
        if (isDirect) {
            emitDirectCall();
        } else {
            emitIndirectCall();
        }
        if (calleeNoReturn) emit(WasmVM::Instr::Unreachable{});
        if (sretBufLocal != -1) {
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)sretBufLocal});
        }
        return;
    }

    // Variadic call: spill extra args onto the shadow stack as i64 slots,
    // pass the spill base as a hidden trailing i64 parameter, restore SP.
    int totalArgs = static_cast<int>(expr.args.size());
    if (totalArgs < namedParamCount) namedParamCount = totalArgs; // recover gracefully
    int numVariadic = totalArgs - namedParamCount;
    size_t spillSize = static_cast<size_t>(numVariadic) * 8;

    // 1. Push named args (left-to-right).
    for (int i = 0; i < namedParamCount; ++i) {
        emitExpr(expr.args[i]);
    }

    // 2. Save current SP into a local.
    int savedSpLocal = allocRawLocal(WasmVM::ValueType::i64);
    emit(WasmVM::Instr::Global_get{kStackPtrIdx});
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)savedSpLocal});

    int spillBaseLocal = savedSpLocal; // when numVariadic == 0
    if (numVariadic > 0) {
        // 3. SP -= spillSize; capture new SP as spill base.
        spillBaseLocal = allocRawLocal(WasmVM::ValueType::i64);
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)savedSpLocal});
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)spillSize});
        emit(WasmVM::Instr::I64_sub{});
        emit(WasmVM::Instr::Local_tee{(WasmVM::index_t)spillBaseLocal});
        emit(WasmVM::Instr::Global_set{kStackPtrIdx});

        // 4. Store each variadic arg as i64 at [spillBase + 8*i] in mem[1].
        for (int i = 0; i < numVariadic; ++i) {
            const auto& varg = expr.args[namedParamCount + i];
            auto vargType = getExprTypeNode(varg);
            auto vargWasmType = vargType ? typeMap_.toWasmType(vargType)
                                         : WasmVM::ValueType::i32;

            if (vargType
                && (vargType->kind == wvmcc::parser::TypeNode::Kind::Struct
                    || vargType->kind == wvmcc::parser::TypeNode::Kind::Union)) {
                wvmcc::Diagnostic d;
                d.severity = wvmcc::Diagnostic::Severity::Error;
                d.message = "variadic struct-by-value arguments not yet supported";
                diagnostics_.push_back(std::move(d));
            }

            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)spillBaseLocal});
            emitExpr(varg);

            // Default argument promotions: char/short→int (already i32),
            // float→double. Then store as i64.
            if (vargWasmType == WasmVM::ValueType::f32) {
                emit(WasmVM::Instr::F64_promote_f32{});
                emit(WasmVM::Instr::I64_reinterpret_f64{});
            } else if (vargWasmType == WasmVM::ValueType::f64) {
                emit(WasmVM::Instr::I64_reinterpret_f64{});
            } else if (vargWasmType == WasmVM::ValueType::i32) {
                emit(WasmVM::Instr::I64_extend_i32_s{});
            }

            emit(WasmVM::Instr::I64_store{
                /*memidx=*/1,
                /*offset=*/static_cast<WasmVM::offset_t>(8 * i),
                /*align=*/3});
        }
    }

    // 5. Push spill base as hidden trailing i64 arg (or savedSp if no variadic args).
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)spillBaseLocal});

    // 6. Emit the call.
    if (isDirect) {
        emitDirectCall();
    } else {
        emitIndirectCall();
    }

    // A no-return variadic callee never comes back; mark the rest unreachable
    // (the SP restore below becomes dead but stays valid under the polymorphic
    // post-`unreachable` stack).
    if (calleeNoReturn) emit(WasmVM::Instr::Unreachable{});

    // 7. Restore SP from saved value if we changed it.
    if (numVariadic > 0) {
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)savedSpLocal});
        emit(WasmVM::Instr::Global_set{kStackPtrIdx});
    }

    if (sretBufLocal != -1) {
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)sretBufLocal});
    }
}

FunctionCodegen::AddrKind FunctionCodegen::addressKind(const wvmcc::parser::Expr* e) {
    using K = wvmcc::parser::Expr::Kind;
    if (!e) return AddrKind::Mem1;
    switch (e->kind) {
        case K::Ident: {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*e);
            auto sym = symbolTable_.lookup(id.name);
            // File-scope variables (GlobalMem) are static mem[0]; address-taken
            // / aggregate locals (MemoryLocal) are static mem[1].
            if (sym && std::holds_alternative<GlobalMem>(*sym)) return AddrKind::Mem0;
            return AddrKind::Mem1;
        }
        case K::Member: {
            const auto& m = static_cast<const wvmcc::parser::MemberExpr&>(*e);
            if (m.isArrow) return AddrKind::Dynamic;     // through a pointer value
            return addressKind(m.base.get());            // `.` follows the base
        }
        case K::Index: {
            const auto& ix = static_cast<const wvmcc::parser::IndexExpr&>(*e);
            // The pointer/array operand may be either side (`2[a]` == `a[2]`).
            auto bt = getExprTypeNode(ix.base);
            const wvmcc::parser::Expr* ptrSide = ix.base.get();
            auto pt = bt;
            auto isPtrArr = [](const wvmcc::parser::TypeNodePtr& t) {
                return t && (t->kind == wvmcc::parser::TypeNode::Kind::Pointer
                             || t->kind == wvmcc::parser::TypeNode::Kind::Array);
            };
            if (!isPtrArr(bt)) {
                auto it = getExprTypeNode(ix.index);
                if (isPtrArr(it)) { ptrSide = ix.index.get(); pt = it; }
            }
            if (pt && pt->kind == wvmcc::parser::TypeNode::Kind::Pointer)
                return AddrKind::Dynamic;                // indexing a pointer value
            return addressKind(ptrSide);                 // array index follows base
        }
        case K::Unary: {
            const auto& u = static_cast<const wvmcc::parser::UnaryExpr&>(*e);
            if (u.op == "*") return AddrKind::Dynamic;   // deref of a pointer value
            return addressKind(u.rhs.get());
        }
        default:
            // Any computed pointer value (call result, cast, pointer arithmetic)
            // is opaque — dispatch on its tag at the access site.
            return AddrKind::Dynamic;
    }
}

// OR the memidx tag onto the i64 address on top of the stack. Only mem[1]
// needs a non-zero nibble; mem[0] and Dynamic (already-tagged) are no-ops.
void FunctionCodegen::emitApplyTag(AddrKind k) {
    if (k != AddrKind::Mem1) return;
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)((int64_t)1 << kMemidxShift)});
    emit(WasmVM::Instr::I64_or{});
}

// [tagged-addr] -> dispatch on nibble, mask off tag, load `type` from mem[0]/[1].
// Uses a void `if` that writes the loaded value into a result local (rather
// than a typed-result block), then leaves it on the stack — this avoids the
// WasmVM interpreter's mishandling of a typed-result block sitting above other
// operands on the value stack.
void FunctionCodegen::emitTaggedLoad(const wvmcc::parser::TypeNodePtr& type) {
    int addrTmp = allocRawLocal(WasmVM::ValueType::i64);
    int resTmp  = allocRawLocal(typeMap_.toWasmType(type));
    emit(WasmVM::Instr::Local_tee{(WasmVM::index_t)addrTmp});
    // nibble (as i32, to avoid the WasmVM i64-compare interpreter trap)
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)kMemidxShift});
    emit(WasmVM::Instr::I64_shr_u{});
    emit(WasmVM::Instr::I32_wrap_i64{});
    emit(WasmVM::Instr::I32_const{1});
    emit(WasmVM::Instr::I32_eq{});
    emit(WasmVM::Instr::If{std::nullopt});
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrTmp});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)kPtrOffMask});
    emit(WasmVM::Instr::I64_and{});
    emit(typeMap_.makeLoad(type, 1));
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)resTmp});
    emit(WasmVM::Instr::Else{});
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrTmp});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)kPtrOffMask});
    emit(WasmVM::Instr::I64_and{});
    emit(typeMap_.makeLoad(type, 0));
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)resTmp});
    emit(WasmVM::Instr::End{});
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)resTmp});
}

// [tagged-addr, value] -> dispatch on nibble, mask off tag, store `type`.
void FunctionCodegen::emitTaggedStore(const wvmcc::parser::TypeNodePtr& type) {
    int valTmp  = allocRawLocal(typeMap_.toWasmType(type));
    int addrTmp = allocRawLocal(WasmVM::ValueType::i64);
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)valTmp});   // pop value
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)addrTmp});  // pop tagged addr
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrTmp});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)kMemidxShift});
    emit(WasmVM::Instr::I64_shr_u{});
    emit(WasmVM::Instr::I32_wrap_i64{});
    emit(WasmVM::Instr::I32_const{1});
    emit(WasmVM::Instr::I32_eq{});
    emit(WasmVM::Instr::If{std::nullopt});
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrTmp});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)kPtrOffMask});
    emit(WasmVM::Instr::I64_and{});
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)valTmp});
    emit(typeMap_.makeStore(type, 1));
    emit(WasmVM::Instr::Else{});
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrTmp});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)kPtrOffMask});
    emit(WasmVM::Instr::I64_and{});
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)valTmp});
    emit(typeMap_.makeStore(type, 0));
    emit(WasmVM::Instr::End{});
}

void FunctionCodegen::emitMemberAccessExpr(const wvmcc::parser::MemberExpr& expr, bool needLValue) {
    // Determine base struct type for field-offset lookup
    auto baseType = getExprTypeNode(expr.base);
    if (expr.isArrow && baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Pointer) {
        baseType = baseType->pointee;
    }

    size_t fieldOffset = baseType ? typeMap_.getFieldOffset(baseType, expr.member) : 0;
    auto fieldType     = baseType ? typeMap_.getFieldType(baseType, expr.member)   : nullptr;
    // `->` is rooted at a pointer value (Dynamic, tag-dispatched); `.` follows
    // the base's storage to a static memory.
    AddrKind k = addressKind(&expr);

    if (expr.isArrow) {
        emitExpr(expr.base, false);  // pointer value (tagged) is the base address
    } else {
        emitExpr(expr.base, true);   // lvalue address of the struct
    }

    if (fieldOffset > 0) {
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)fieldOffset});
        emit(WasmVM::Instr::I64_add{});
    }

    if (!needLValue) {
        if (k == AddrKind::Dynamic) emitTaggedLoad(fieldType);
        else emit(typeMap_.makeLoad(fieldType, k == AddrKind::Mem1 ? 1 : 0));
    }
}

void FunctionCodegen::emitArrayIndexExpr(const wvmcc::parser::IndexExpr& expr, bool needLValue) {
    // E1[E2] is *((E1)+(E2)) (6.5.2.1p2), and addition is commutative, so the
    // pointer/array operand may be either side — `2[a]` means `a[2]`. Pick the
    // pointer/array operand as the base and the other as the index.
    const wvmcc::parser::ExprPtr* baseE  = &expr.base;
    const wvmcc::parser::ExprPtr* indexE = &expr.index;
    auto baseType = getExprTypeNode(expr.base);
    auto isPtrArr = [](const wvmcc::parser::TypeNodePtr& t) {
        return t && (t->kind == wvmcc::parser::TypeNode::Kind::Pointer
                     || t->kind == wvmcc::parser::TypeNode::Kind::Array);
    };
    if (!isPtrArr(baseType)) {
        auto idxType = getExprTypeNode(expr.index);
        if (isPtrArr(idxType)) { std::swap(baseE, indexE); baseType = idxType; }
    }

    wvmcc::parser::TypeNodePtr elemType;
    bool baseIsPointer = false;
    if (baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Pointer) {
        elemType = baseType->pointee;
        baseIsPointer = true;
    } else if (baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Array) {
        elemType = baseType->element;
    }

    size_t elemSize = elemType ? typeMap_.byteSize(elemType) : 4;
    // Indexing a pointer is rooted at a pointer value (Dynamic, tag-dispatched);
    // indexing an array follows the base's storage to a static memory.
    AddrKind k = addressKind(&expr);

    // Base address (i64). For an array base, take its untagged lvalue address
    // (need_lvalue=true) rather than the tagged decayed pointer; for a pointer
    // base, load the pointer *value* (which carries its own tag).
    emitExpr(*baseE, /*needLValue=*/!baseIsPointer);
    emitExpr(*indexE, false);   // index

    auto idxWasmType = getExprType(*indexE);
    if (idxWasmType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I64_extend_i32_s{});
    }

    if (elemSize > 1) {
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)elemSize});
        emit(WasmVM::Instr::I64_mul{});
    }

    emit(WasmVM::Instr::I64_add{});

    if (!needLValue) {
        if (k == AddrKind::Dynamic) emitTaggedLoad(elemType);
        else emit(typeMap_.makeLoad(elemType, k == AddrKind::Mem1 ? 1 : 0));
    }
}

void FunctionCodegen::emitListInitializer(int baseAddrLocal,
                                          const wvmcc::parser::TypeNodePtr& type,
                                          const wvmcc::parser::InitializerPtr& init,
                                          uint8_t memidx) {
    using namespace wvmcc::parser;
    if (!type || !init || init->kind != Initializer::Kind::List) return;

    auto emitBase = [&](size_t off) {
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)baseAddrLocal});
        if (off > 0) {
            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)off});
            emit(WasmVM::Instr::I64_add{});
        }
    };

    if ((type->kind == TypeNode::Kind::Struct || type->kind == TypeNode::Kind::Union) && type->su) {
        // Walk fields in declaration order. For each field, look for a clause
        // with a matching `.field` designator first, then fall back to the
        // next positional clause.
        size_t posIdx = 0;
        for (const auto& member : type->su->members) {
            for (const auto& sd : member.declarators) {
                if (!sd.declarator) continue;
                std::string fieldName = sd.declarator->id.name;
                if (fieldName.empty()) continue;

                const InitClause* clause = nullptr;
                for (const auto& cl : init->clauses) {
                    if (!cl.designators.empty()
                        && cl.designators[0].kind == Designator::Kind::Member
                        && cl.designators[0].member == fieldName) {
                        clause = &cl;
                        break;
                    }
                }
                if (!clause) {
                    // Advance positional index past any leading designated clauses.
                    while (posIdx < init->clauses.size()
                           && !init->clauses[posIdx].designators.empty()) {
                        ++posIdx;
                    }
                    if (posIdx < init->clauses.size()) {
                        clause = &init->clauses[posIdx++];
                    }
                }
                if (!clause || !clause->init) continue;

                size_t fieldOff = typeMap_.getFieldOffset(type, fieldName);
                auto fieldType = typeMap_.getFieldType(type, fieldName);

                if (clause->init->kind == Initializer::Kind::Expr && clause->init->expr) {
                    emitBase(fieldOff);
                    emitExpr(clause->init->expr);
                    // Convert to the member type (as by assignment) so e.g. the
                    // literal `0` (i32) into a pointer/long member (i64) extends
                    // rather than type-mismatching the store.
                    emitConvert(this, getExprType(clause->init->expr), typeMap_.toWasmType(fieldType));
                    emit(typeMap_.makeStore(fieldType, memidx));
                } else if (clause->init->kind == Initializer::Kind::List && fieldType) {
                    // Nested aggregate: recurse with a base = baseAddr + fieldOff.
                    int subAddr = allocRawLocal(WasmVM::ValueType::i64);
                    emitBase(fieldOff);
                    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)subAddr});
                    emitListInitializer(subAddr, fieldType, clause->init, memidx);
                }
            }
        }
    } else if (type->kind == TypeNode::Kind::Array && type->element) {
        size_t elemSize = typeMap_.byteSize(type->element);
        size_t curIdx = 0;
        for (const auto& cl : init->clauses) {
            if (!cl.designators.empty()
                && cl.designators[0].kind == Designator::Kind::Index
                && cl.designators[0].index.has_value()) {
                auto v = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(*cl.designators[0].index);
                if (v.has_value() && *v >= 0) curIdx = (size_t)*v;
            }
            if (!cl.init) { ++curIdx; continue; }

            size_t off = curIdx * elemSize;
            if (cl.init->kind == Initializer::Kind::Expr && cl.init->expr) {
                emitBase(off);
                emitExpr(cl.init->expr);
                emitConvert(this, getExprType(cl.init->expr), typeMap_.toWasmType(type->element));
                emit(typeMap_.makeStore(type->element, memidx));
            } else if (cl.init->kind == Initializer::Kind::List) {
                int subAddr = allocRawLocal(WasmVM::ValueType::i64);
                emitBase(off);
                emit(WasmVM::Instr::Local_set{(WasmVM::index_t)subAddr});
                emitListInitializer(subAddr, type->element, cl.init, memidx);
            }
            ++curIdx;
        }
    }
}

void FunctionCodegen::emitCompoundLiteralExpr(const wvmcc::parser::CompoundLiteral& expr) {
    if (!expr.type) {
        emitUnimplemented("codegen: compound literal missing type", expr.span);
        return;
    }

    size_t size  = typeMap_.byteSize(expr.type);
    size_t align = typeMap_.byteAlignment(expr.type);

    if (framePointerLocal_ == -1) {
        framePointerLocal_ = allocRawLocal(WasmVM::ValueType::i64);
    }
    if (align > 1) frameSize_ = (frameSize_ + align - 1) & ~(align - 1);
    size_t bufOff = frameSize_;
    frameSize_ += size;

    // Compute and stash temp address: fp + bufOff
    int addrLocal = allocRawLocal(WasmVM::ValueType::i64);
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)bufOff});
    emit(WasmVM::Instr::I64_add{});
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)addrLocal});

    bool isList = expr.init && expr.init->kind == wvmcc::parser::Initializer::Kind::List;
    bool isStruct = expr.type->kind == wvmcc::parser::TypeNode::Kind::Struct
                    || expr.type->kind == wvmcc::parser::TypeNode::Kind::Union;

    if (isList && isStruct && expr.type->su) {
        // Struct/union list initializer: assign fields in declaration order.
        // Match positionally (or by member designator).
        size_t clauseIdx = 0;
        for (const auto& member : expr.type->su->members) {
            for (const auto& sd : member.declarators) {
                if (!sd.declarator || sd.declarator->id.name.empty()) continue;
                const std::string& fieldName = sd.declarator->id.name;

                const wvmcc::parser::InitClause* clause = nullptr;
                // Prefer member designator match
                for (const auto& cl : expr.init->clauses) {
                    if (!cl.designators.empty()
                        && cl.designators[0].kind == wvmcc::parser::Designator::Kind::Member
                        && cl.designators[0].member == fieldName) {
                        clause = &cl;
                        break;
                    }
                }
                // Fall back to positional
                if (!clause && clauseIdx < expr.init->clauses.size()) {
                    const auto& cl = expr.init->clauses[clauseIdx];
                    if (cl.designators.empty()) clause = &cl;
                }
                if (clause) ++clauseIdx;
                if (!clause || !clause->init
                    || clause->init->kind != wvmcc::parser::Initializer::Kind::Expr
                    || !clause->init->expr) continue;

                auto fieldType = typeMap_.getFieldType(expr.type, fieldName);
                size_t fieldOff = typeMap_.getFieldOffset(expr.type, fieldName);

                emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrLocal});
                if (fieldOff > 0) {
                    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)fieldOff});
                    emit(WasmVM::Instr::I64_add{});
                }
                emitExpr(clause->init->expr);
                // Convert the initializer to the member's type (as by
                // assignment): e.g. the literal `0` (i32) into a pointer/long
                // member (i64) needs an extend, or the store type-mismatches.
                emitConvert(this, getExprType(clause->init->expr), typeMap_.toWasmType(fieldType));
                emit(typeMap_.makeStore(fieldType, 1));
            }
        }
    } else if (isList) {
        // Scalar list initializer: use first clause only
        if (!expr.init->clauses.empty()) {
            const auto& cl = expr.init->clauses[0];
            if (cl.init && cl.init->kind == wvmcc::parser::Initializer::Kind::Expr && cl.init->expr) {
                emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrLocal});
                emitExpr(cl.init->expr);
                emit(typeMap_.makeStore(expr.type, 1));
            }
        }
    } else if (expr.init && expr.init->kind == wvmcc::parser::Initializer::Kind::Expr && expr.init->expr) {
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrLocal});
        emitExpr(expr.init->expr);
        emit(typeMap_.makeStore(expr.type, 1));
    }

    // Leave address on stack as the expression value
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)addrLocal});
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
    case K::DoWhile:
        emitDoWhileStmt(static_cast<const wvmcc::parser::DoWhileStmt&>(*stmt));
        break;
    case K::Switch:
        emitSwitchStmt(static_cast<const wvmcc::parser::SwitchStmt&>(*stmt));
        break;
    case K::Break:
        emitBreakStmt(static_cast<const wvmcc::parser::BreakStmt&>(*stmt));
        break;
    case K::Continue:
        emitContinueStmt(static_cast<const wvmcc::parser::ContinueStmt&>(*stmt));
        break;
    case K::Goto:
        emitGotoStmt(static_cast<const wvmcc::parser::GotoStmt&>(*stmt));
        break;
    case K::Label:
        emitLabelStmt(static_cast<const wvmcc::parser::LabelStmt&>(*stmt));
        break;
    case K::Case:
    case K::Default:
        // Stray case/default outside a switch: emit body only.
        if (stmt->kind == K::Case) {
            emitStmt(static_cast<const wvmcc::parser::CaseStmt&>(*stmt).stmt);
        } else {
            emitStmt(static_cast<const wvmcc::parser::DefaultStmt&>(*stmt).stmt);
        }
        break;
    case K::Empty:
        break;
    default:
        emitUnimplemented("codegen not implemented: statement kind " + std::to_string((int)stmt->kind), stmt->span);
        break;
    }
}

// Walk a (possibly nested) declarator and extract the bound identifier.
static std::string declaratorBoundName(const wvmcc::parser::DeclaratorPtr& d) {
    auto cur = d;
    while (cur) {
        if (!cur->id.name.empty()) return cur->id.name;
        if (cur->inner.has_value()) cur = *cur->inner;
        else break;
    }
    return std::string();
}

void FunctionCodegen::emitBlockItem(const wvmcc::parser::BlockItemPtr& item) {
    if (!item) return;

    std::visit([this](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, wvmcc::parser::DeclarationPtr>) {
            if (!v || !v->declarator) return;
            std::string name = declaratorBoundName(v->declarator);
            if (name.empty()) return;

            // Prefer Semantic::canonicalTypeRepr (handles pointer-to-function,
            // arrays, etc. AND resolves typedef-named types to their underlying
            // struct/union so member access on locals like `FILE *f` works).
            // Fall back to a hand-built TypeNode for tests that don't wire up a
            // Semantic instance.
            wvmcc::parser::TypeNodePtr typeNode;
            if (semantic_) {
                typeNode = semantic_->canonicalTypeRepr(v->specifiers, v->declarator);
            }
            if (!typeNode) {
                for (const auto& ts : v->specifiers.typeSpecifiers) {
                    if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                        && !ts.simple.empty()) {
                        auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                        node->kind = wvmcc::parser::TypeNode::Kind::Builtin;
                        node->simple = ts.simple;
                        typeNode = node;
                        break;
                    } else if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                               && ts.su) {
                        auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                        node->kind = (ts.su->kind == wvmcc::parser::StructOrUnionSpecifier::Kind::Struct)
                                     ? wvmcc::parser::TypeNode::Kind::Struct
                                     : wvmcc::parser::TypeNode::Kind::Union;
                        node->su = ts.su;
                        typeNode = node;
                        break;
                    }
                }
                if (typeNode && v->declarator->inner.has_value()
                    && *v->declarator->inner
                    && (*v->declarator->inner)->kind == wvmcc::parser::Declarator::Kind::Pointer) {
                    auto ptrNode = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    ptrNode->kind = wvmcc::parser::TypeNode::Kind::Pointer;
                    ptrNode->pointee = typeNode;
                    typeNode = ptrNode;
                }
            }

            // Static local: allocate in mem[0] and emit a one-time init guard.
            bool isStatic = v->specifiers.hasStorage(wvmcc::parser::StorageClass::Static);
            if (isStatic && moduleCg_) {
                size_t size  = typeMap_.byteSize(typeNode);
                size_t align = typeMap_.byteAlignment(typeNode);
                if (size == 0) size = 4;
                if (align == 0) align = 4;
                size_t addr = moduleCg_->allocateStaticStorage(size, align);

                GlobalMem gm;
                gm.type = typeNode;
                gm.dataSegmentIndex = -1;
                gm.address = addr;
                symbolTable_.define(name, gm);

                if (v->initializer && *v->initializer) {
                    WasmVM::index_t guard = moduleCg_->allocateGuardGlobal();
                    // if (!guard) { <init>; guard = 1; }
                    emit(WasmVM::Instr::Global_get{guard});
                    emit(WasmVM::Instr::I32_eqz{});
                    emit(WasmVM::Instr::If{std::nullopt});
                    if ((*v->initializer)->kind == wvmcc::parser::Initializer::Kind::Expr
                        && (*v->initializer)->expr) {
                        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)addr});
                        emitExpr((*v->initializer)->expr);
                        emit(typeMap_.makeStore(typeNode, 0));
                    } else if ((*v->initializer)->kind == wvmcc::parser::Initializer::Kind::List) {
                        int baseAddr = allocRawLocal(WasmVM::ValueType::i64);
                        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)addr});
                        emit(WasmVM::Instr::Local_set{(WasmVM::index_t)baseAddr});
                        emitListInitializer(baseAddr, typeNode, *v->initializer, 0);
                    }
                    emit(WasmVM::Instr::I32_const{1});
                    emit(WasmVM::Instr::Global_set{guard});
                    emit(WasmVM::Instr::End{});
                }
                return;
            }

            bool isAddrTaken = addressTakenNames_.count(name) > 0;
            // Struct/union/array variables are always memory-resident.
            bool isAggregateType = typeNode
                && (typeNode->kind == wvmcc::parser::TypeNode::Kind::Struct
                    || typeNode->kind == wvmcc::parser::TypeNode::Kind::Union
                    || typeNode->kind == wvmcc::parser::TypeNode::Kind::Array);
            int slotOrOffset = allocLocal(typeNode, isAddrTaken || isAggregateType);

            if (isAddrTaken || isAggregateType) {
                MemoryLocal info;
                info.type = typeNode;
                info.frameOffset = (size_t)slotOrOffset;
                symbolTable_.define(name, info);

                if (v->initializer && *v->initializer) {
                    if ((*v->initializer)->kind == wvmcc::parser::Initializer::Kind::Expr
                        && (*v->initializer)->expr) {
                        const auto& srcExpr = (*v->initializer)->expr;
                        if (isAggregateType) {
                            // Aggregate copy-initialization (`struct q = <rvalue>;`):
                            // the source expression yields the object's address;
                            // copy its bytes into this frame slot. (A scalar
                            // makeStore here would store the *address*, not the
                            // value — the bug that made `struct b = a;` corrupt.)
                            emitExpr(srcExpr, /*needLValue=*/true);
                            int srcAddr = allocRawLocal(WasmVM::ValueType::i64);
                            emit(WasmVM::Instr::Local_set{(WasmVM::index_t)srcAddr});
                            int dstAddr = allocRawLocal(WasmVM::ValueType::i64);
                            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
                            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.frameOffset});
                            emit(WasmVM::Instr::I64_add{});
                            emit(WasmVM::Instr::Local_set{(WasmVM::index_t)dstAddr});
                            AddrKind sk = addressKind(srcExpr.get());
                            uint8_t srcMem = (sk == AddrKind::Mem0) ? 0 : 1;
                            emitBytewiseCopy(dstAddr, /*dstMemidx=*/1, srcAddr, srcMem,
                                             typeMap_.byteSize(typeNode));
                        } else {
                            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
                            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.frameOffset});
                            emit(WasmVM::Instr::I64_add{});
                            emitExpr(srcExpr);
                            emit(typeMap_.makeStore(typeNode, 1));
                        }
                    } else if ((*v->initializer)->kind == wvmcc::parser::Initializer::Kind::List) {
                        int baseAddr = allocRawLocal(WasmVM::ValueType::i64);
                        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
                        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.frameOffset});
                        emit(WasmVM::Instr::I64_add{});
                        emit(WasmVM::Instr::Local_set{(WasmVM::index_t)baseAddr});
                        emitListInitializer(baseAddr, typeNode, *v->initializer, 1);
                    }
                }
            } else {
                ScalarLocal info;
                info.type = typeNode;
                info.isAddressTaken = false;
                info.localIndex = slotOrOffset;
                symbolTable_.define(name, info);

                if (v->initializer
                    && (*v->initializer)->kind == wvmcc::parser::Initializer::Kind::Expr
                    && (*v->initializer)->expr) {
                    emitExpr((*v->initializer)->expr);
                    // Convert initializer to local's declared type if needed.
                    auto rhsVt = getExprType((*v->initializer)->expr);
                    auto lhsVt = typeNode ? typeMap_.toWasmType(typeNode)
                                          : WasmVM::ValueType::i32;
                    emitConvert(this, rhsVt, lhsVt);
                    // _Bool: normalize the stored value to 0 or 1.
                    if (typeNode && typeNode->kind == wvmcc::parser::TypeNode::Kind::Builtin
                        && !typeNode->simple.empty()
                        && typeNode->simple[0]
                           == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Bool) {
                        emit(WasmVM::Instr::I32_const{0});
                        emit(WasmVM::Instr::I32_ne{});
                    }
                    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)slotOrOffset});
                }
            }
        } else if constexpr (std::is_same_v<T, wvmcc::parser::StmtPtr>) {
            emitStmt(v);
        }
        // StaticAssert: ignored in codegen
    }, item->item);
}

void FunctionCodegen::emitReturnStmt(const wvmcc::parser::ReturnStmt& stmt) {
    if (hiddenRetPtrLocal_ != -1) {
        // Struct return: copy value to hidden sret pointer, no Wasm result.
        if (stmt.value) {
            emitStructCopyToHiddenPtr(*stmt.value);
        }
    } else if (stmt.value) {
        emitExpr(*stmt.value);
        // The return value is converted as if by assignment to the function's
        // return type (6.8.6.4): coerce its Wasm value type (incl. float<->int,
        // i32<->i64) and reduce to the target width, so e.g. `return 3.75;` in
        // an int function truncates and `return 260;` in an unsigned-char
        // function wraps — otherwise the validator rejects the type mismatch.
        if (returnWasmType_.has_value()) {
            auto srcVt = getExprType(*stmt.value);
            emitConvert(this, srcVt, *returnWasmType_);
            if (isBoolTypeNode(returnScalarType_)) {
                emit(WasmVM::Instr::I32_const{0});
                emit(WasmVM::Instr::I32_ne{});
            } else {
                emitIntegerNarrow(returnScalarType_);
            }
        }
    }
    if (framePointerLocal_ != -1 && frameSize_ > 0) {
        generateEpilogue();
    }
    emit(WasmVM::Instr::Return{});
}

void FunctionCodegen::emitStructCopyToHiddenPtr(const wvmcc::parser::ExprPtr& srcExpr) {
    if (!returnTypeNode_ || !returnTypeNode_->su) {
        emitUnimplemented("codegen: struct return missing layout info",
                          srcExpr ? srcExpr->span : std::optional<wvmcc::SourceSpan>{});
        return;
    }

    // Get the source address (lvalue address on shadow stack).
    emitExpr(srcExpr, true);
    int srcAddrLocal = allocRawLocal(WasmVM::ValueType::i64);
    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)srcAddrLocal});

    // Field-by-field copy: load from mem[1] (shadow stack), store to mem[0] (via hidden ptr).
    for (const auto& member : returnTypeNode_->su->members) {
        for (const auto& sd : member.declarators) {
            if (!sd.declarator || sd.declarator->id.name.empty()) continue;
            const std::string& fieldName = sd.declarator->id.name;
            auto fieldType = typeMap_.getFieldType(returnTypeNode_, fieldName);
            if (!fieldType) continue;
            size_t fieldOff = typeMap_.getFieldOffset(returnTypeNode_, fieldName);

            // dst = hidden_ptr + fieldOff
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)hiddenRetPtrLocal_});
            if (fieldOff > 0) {
                emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)fieldOff});
                emit(WasmVM::Instr::I64_add{});
            }
            // value = load field from src (shadow stack, mem[1])
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)srcAddrLocal});
            if (fieldOff > 0) {
                emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)fieldOff});
                emit(WasmVM::Instr::I64_add{});
            }
            emit(typeMap_.makeLoad(fieldType, 1));
            // store to hidden ptr (mem[0])
            emit(typeMap_.makeStore(fieldType, 0));
        }
    }
}

// #79: does an indirect call through `callee` (a function-pointer expression)
// leave a value on the stack? False for a `void` return (call_indirect pushes
// nothing, so the expression-statement path must not emit a Drop). Struct
// returns currently still leave the sret pointer.
void FunctionCodegen::emitBytewiseCopy(int dstAddrLocal, uint8_t dstMemidx,
                                       int srcAddrLocal, uint8_t srcMemidx, size_t size) {
    using O = WasmVM::offset_t;
    auto copy = [&](WasmVM::WasmInstr ld, WasmVM::WasmInstr st) {
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)dstAddrLocal});
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)srcAddrLocal});
        emit(ld);   // load chunk from src (uses its own byte offset)
        emit(st);   // store chunk to dst (consumes [dstAddr, value])
    };
    size_t off = 0;
    while (size - off >= 8) {
        copy(WasmVM::Instr::I64_load{srcMemidx, (O)off, 3}, WasmVM::Instr::I64_store{dstMemidx, (O)off, 3});
        off += 8;
    }
    if (size - off >= 4) {
        copy(WasmVM::Instr::I32_load{srcMemidx, (O)off, 2}, WasmVM::Instr::I32_store{dstMemidx, (O)off, 2});
        off += 4;
    }
    if (size - off >= 2) {
        copy(WasmVM::Instr::I32_load16_u{srcMemidx, (O)off, 1}, WasmVM::Instr::I32_store16{dstMemidx, (O)off, 1});
        off += 2;
    }
    if (size - off >= 1) {
        copy(WasmVM::Instr::I32_load8_u{srcMemidx, (O)off, 0}, WasmVM::Instr::I32_store8{dstMemidx, (O)off, 0});
        off += 1;
    }
}

bool FunctionCodegen::indirectCallLeavesValue(const wvmcc::parser::ExprPtr& callee) {
    auto ct = getExprTypeNode(callee);
    auto fn = ct;
    if (fn && fn->kind == wvmcc::parser::TypeNode::Kind::Pointer) fn = fn->pointee;
    if (!fn || fn->kind != wvmcc::parser::TypeNode::Kind::Function) return true;
    auto ret = fn->element;
    bool isVoidRet = !ret
        || (ret->kind == wvmcc::parser::TypeNode::Kind::Builtin
            && !ret->simple.empty()
            && ret->simple[0]
               == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void);
    return !isVoidRet;
}

// Whether evaluating `expr` for its value pushes exactly one result onto the
// stack (so a discarding context must Drop it). Almost everything does; the
// exceptions are void/struct-returning calls and the void va_* builtins.
bool FunctionCodegen::exprLeavesValue(const wvmcc::parser::ExprPtr& expr) {
    if (!expr) return false;
    if (expr->kind != wvmcc::parser::Expr::Kind::Call) return true;
    const auto& call = static_cast<const wvmcc::parser::CallExpr&>(*expr);
    if (call.callee && call.callee->kind == wvmcc::parser::Expr::Kind::Ident) {
        const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*call.callee);
        if (id.name == "__builtin_va_start" || id.name == "__builtin_va_end"
            || id.name == "__builtin_va_copy")
            return false;
        if (id.name == "__builtin_va_arg") return true;
        auto funcSym = symbolTable_.lookupFunction(id.name);
        if (funcSym) {
            bool isVoidRet = !funcSym->type
                || (funcSym->type->kind == wvmcc::parser::TypeNode::Kind::Builtin
                    && !funcSym->type->simple.empty()
                    && funcSym->type->simple[0]
                       == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void);
            bool isStructRet = funcSym->type
                && (funcSym->type->kind == wvmcc::parser::TypeNode::Kind::Struct
                    || funcSym->type->kind == wvmcc::parser::TypeNode::Kind::Union);
            return !isVoidRet && !isStructRet;
        }
        // #79: not a direct call — the identifier names a function-pointer
        // variable. Decide from its pointee type.
        return indirectCallLeavesValue(call.callee);
    }
    // #79: indirect call through an arbitrary function-pointer expression.
    return indirectCallLeavesValue(call.callee);
}

void FunctionCodegen::emitExprStmt(const wvmcc::parser::ExprStmt& stmt) {
    if (!stmt.expr) return;
    emitExpr(stmt.expr);
    if (exprLeavesValue(stmt.expr)) {
        emit(WasmVM::Instr::Drop{});
    }
}

wvmcc::parser::ExprPtr FunctionCodegen::selectGenericAssociation(
    const wvmcc::parser::GenericSelectionExpr& g) const {
    auto canon = [](wvmcc::parser::TypeNodePtr r) {
        // Lvalue conversion drops top-level cv-qualifiers (6.3.2.1p2).
        while (r && r->kind == wvmcc::parser::TypeNode::Kind::Qualified && r->pointee)
            r = r->pointee;
        return r;
    };
    auto ctrlType = canon(getExprTypeNode(g.controlling));
    for (const auto& assoc : g.assocs) {
        if (!assoc.isDefault && assoc.type
            && wvmcc::parser::Semantic::typeNodesEqual(canon(assoc.type), ctrlType))
            return assoc.expr;
    }
    for (const auto& assoc : g.assocs)
        if (assoc.isDefault) return assoc.expr;
    return nullptr;
}

void FunctionCodegen::emitItemsWithGotoLift(const std::vector<wvmcc::parser::BlockItemPtr>& items) {
    using K = wvmcc::parser::Stmt::Kind;

    // Pre-pass: collect labels at this level (label name → item index).
    std::unordered_map<std::string, size_t> labelIdx;
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        if (!item) continue;
        if (auto sp = std::get_if<wvmcc::parser::StmtPtr>(&item->item)) {
            if (*sp && (*sp)->kind == K::Label) {
                const auto& ls = static_cast<const wvmcc::parser::LabelStmt&>(**sp);
                labelIdx[ls.name] = i;
            }
        }
    }

    // Stack of currently-open forward-goto wrapper Blocks. Each entry records
    // the item index at which the wrapping Block must close.
    struct OpenBlock { size_t closeAt; };
    std::vector<OpenBlock> openBlocks;

    for (size_t i = 0; i < items.size(); ++i) {
        while (!openBlocks.empty() && openBlocks.back().closeAt == i) {
            emit(WasmVM::Instr::End{});
            openBlocks.pop_back();
        }

        const auto& item = items[i];
        if (!item) continue;

        if (auto sp = std::get_if<wvmcc::parser::StmtPtr>(&item->item)) {
            if (*sp && (*sp)->kind == K::Goto) {
                const auto& gs = static_cast<const wvmcc::parser::GotoStmt&>(**sp);
                auto it = labelIdx.find(gs.label);
                if (it != labelIdx.end() && it->second > i) {
                    if (!openBlocks.empty() && it->second > openBlocks.back().closeAt) {
                        emit(WasmVM::Instr::Unreachable{});
                        wvmcc::Diagnostic d;
                        d.severity = wvmcc::Diagnostic::Severity::Error;
                        d.message = "forward goto with overlapping range is not supported";
                        diagnostics_.push_back(std::move(d));
                        continue;
                    }
                    emit(WasmVM::Instr::Block{std::nullopt});
                    openBlocks.push_back({it->second});
                    emit(WasmVM::Instr::Br{0});
                    continue;
                }
            }
        }

        emitBlockItem(item);
    }

    while (!openBlocks.empty()) {
        emit(WasmVM::Instr::End{});
        openBlocks.pop_back();
    }
}

void FunctionCodegen::emitCompoundStmt(const wvmcc::parser::CompoundStmt& stmt) {
    symbolTable_.pushScope();
    emitItemsWithGotoLift(stmt.items);
    symbolTable_.popScope();
}

void FunctionCodegen::emitIfStmt(const wvmcc::parser::IfStmt& stmt) {
    emitExpr(stmt.cond);
    // Wasm's `if` consumes an i32 condition. If the C condition is an i64
    // (pointer, long), collapse it to a 0/1 i32 via two `eqz`s — simpler
    // than emitting an explicit compare-with-zero and reads the same way.
    if (getExprType(stmt.cond) == WasmVM::ValueType::i64) {
        emit(WasmVM::Instr::I64_eqz{});
        emit(WasmVM::Instr::I32_eqz{});
    }
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
    //   loop $top   (= $continue: re-evaluates the condition)
    //     <cond>; i32.eqz; br_if $break
    //     <body>
    //     br $top
    //   end
    // end
    emit(WasmVM::Instr::Block{std::nullopt});
    int breakD = currentBlockDepth_;
    emit(WasmVM::Instr::Loop{std::nullopt});
    int contD = currentBlockDepth_;
    pushLoop(breakD, contD);
    emitExpr(stmt.cond);
    emit(WasmVM::Instr::I32_eqz{});
    emit(WasmVM::Instr::Br_if{breakDepth()});
    emitStmt(stmt.body);
    emit(WasmVM::Instr::Br{continueDepth()});
    popControlFlow();
    emit(WasmVM::Instr::End{});
    emit(WasmVM::Instr::End{});
}

void FunctionCodegen::emitForStmt(const wvmcc::parser::ForStmt& stmt) {
    // block $break
    //   loop $loop
    //     <cond>; i32.eqz; br_if $break
    //     block $continue
    //       <body>            ; `continue` => br $continue
    //     end                 ; falls through to the step
    //     <step>
    //     br $loop            ; repeat (re-evaluate <cond>)
    //   end
    // end
    // C requires `continue` to run the loop's step before re-testing the
    // condition. The dedicated $continue block makes `continue` land *before*
    // the step (an earlier version branched to the loop top and skipped the
    // step, hanging any counting loop that used `continue`).
    symbolTable_.pushScope();
    if (stmt.init) {
        emitBlockItem(*stmt.init);
    }
    emit(WasmVM::Instr::Block{std::nullopt});
    int breakD = currentBlockDepth_;
    emit(WasmVM::Instr::Loop{std::nullopt});
    int loopD = currentBlockDepth_;
    if (stmt.cond) {
        emitExpr(*stmt.cond);
        emit(WasmVM::Instr::I32_eqz{});
        emit(WasmVM::Instr::Br_if{(WasmVM::index_t)(currentBlockDepth_ - breakD)});
    }
    emit(WasmVM::Instr::Block{std::nullopt});
    int contD = currentBlockDepth_;
    pushLoop(breakD, contD);
    emitStmt(stmt.body);
    popControlFlow();
    emit(WasmVM::Instr::End{}); // end $continue — `continue` lands here
    if (stmt.step) {
        emitExpr(*stmt.step);
        emit(WasmVM::Instr::Drop{});
    }
    emit(WasmVM::Instr::Br{(WasmVM::index_t)(currentBlockDepth_ - loopD)});
    emit(WasmVM::Instr::End{}); // end $loop
    emit(WasmVM::Instr::End{}); // end $break
    symbolTable_.popScope();
}

void FunctionCodegen::emitDoWhileStmt(const wvmcc::parser::DoWhileStmt& stmt) {
    // block $break
    //   loop $continue
    //     <body>
    //     <cond>; br_if $continue   (loop back if true)
    //   end
    // end
    // Note: `continue` re-enters the loop top, skipping the cond test.
    // The Phase 3 issue specifies this simpler shape.
    emit(WasmVM::Instr::Block{std::nullopt});
    int breakD = currentBlockDepth_;
    emit(WasmVM::Instr::Loop{std::nullopt});
    int contD = currentBlockDepth_;
    pushLoop(breakD, contD);
    emitStmt(stmt.body);
    emitExpr(stmt.cond);
    emit(WasmVM::Instr::Br_if{continueDepth()});
    popControlFlow();
    emit(WasmVM::Instr::End{});
    emit(WasmVM::Instr::End{});
}

void FunctionCodegen::emitBreakStmt(const wvmcc::parser::BreakStmt&) {
    if (controlFlowStack_.empty()) {
        emit(WasmVM::Instr::Unreachable{});
        wvmcc::Diagnostic d;
        d.severity = wvmcc::Diagnostic::Severity::Error;
        d.message = "'break' not inside loop or switch";
        diagnostics_.push_back(std::move(d));
        return;
    }
    emit(WasmVM::Instr::Br{breakDepth()});
}

void FunctionCodegen::emitContinueStmt(const wvmcc::parser::ContinueStmt&) {
    // Continue requires an enclosing loop (skipping switches).
    auto stk = controlFlowStack_;
    bool hasLoop = false;
    while (!stk.empty()) {
        if (stk.top().kind == ControlFlowEntry::Loop) { hasLoop = true; break; }
        stk.pop();
    }
    if (!hasLoop) {
        emit(WasmVM::Instr::Unreachable{});
        wvmcc::Diagnostic d;
        d.severity = wvmcc::Diagnostic::Severity::Error;
        d.message = "'continue' not inside a loop";
        diagnostics_.push_back(std::move(d));
        return;
    }
    emit(WasmVM::Instr::Br{continueDepth()});
}

void FunctionCodegen::emitLabelStmt(const wvmcc::parser::LabelStmt& stmt) {
    // The block wrapping a forward-goto target is emitted in emitCompoundStmt;
    // here we just emit the inner statement.
    emitStmt(stmt.stmt);
}

void FunctionCodegen::emitGotoStmt(const wvmcc::parser::GotoStmt&) {
    // Forward gotos are lifted in emitCompoundStmt and emitted there as Br.
    // Reaching this method means the goto was not lifted (backward goto, or
    // target outside the current compound-statement scope).
    emit(WasmVM::Instr::Unreachable{});
    wvmcc::Diagnostic d;
    d.severity = wvmcc::Diagnostic::Severity::Error;
    d.message = "backward or non-local goto is not supported";
    diagnostics_.push_back(std::move(d));
}

namespace {
struct CaseEntry {
    long long value;
    int segmentIdx;     // index into segments[]
};
struct Segment {
    bool isDefault = false;
    std::vector<long long> caseValues;
    std::vector<wvmcc::parser::StmtPtr> body;
};

// Walk a switch body and partition it into segments.
// One segment per case/default label (chained labels each get their own
// segment with empty body, falling through to the next).
// Statements before the first case label are dropped (unreachable).
static void collectSwitchSegments(const wvmcc::parser::StmtPtr& body,
                                  std::vector<Segment>& segments,
                                  int& defaultSegIdx,
                                  std::vector<CaseEntry>& cases,
                                  std::vector<wvmcc::Diagnostic>& diags) {
    using namespace wvmcc::parser;
    if (!body) return;

    std::function<void(const StmtPtr&)> process = [&](const StmtPtr& s) {
        if (!s) return;
        if (s->kind == Stmt::Kind::Case) {
            const auto& cs = static_cast<const CaseStmt&>(*s);
            segments.emplace_back();
            auto v = ConstExprEvaluator::evalIntegerConstantExpr(cs.value);
            if (!v.has_value()) {
                wvmcc::Diagnostic d;
                d.severity = wvmcc::Diagnostic::Severity::Error;
                d.message = "case label requires an integer constant expression";
                diags.push_back(std::move(d));
            } else {
                segments.back().caseValues.push_back(*v);
                cases.push_back({*v, (int)segments.size() - 1});
            }
            process(cs.stmt);
        } else if (s->kind == Stmt::Kind::Default) {
            const auto& ds = static_cast<const DefaultStmt&>(*s);
            segments.emplace_back();
            segments.back().isDefault = true;
            if (defaultSegIdx == -1) defaultSegIdx = (int)segments.size() - 1;
            process(ds.stmt);
        } else {
            if (segments.empty()) return;  // pre-label stmt: unreachable
            segments.back().body.push_back(s);
        }
    };

    if (body->kind == Stmt::Kind::Compound) {
        const auto& cs = static_cast<const CompoundStmt&>(*body);
        for (const auto& item : cs.items) {
            if (auto stmtPtr = std::get_if<StmtPtr>(&item->item)) {
                if (*stmtPtr) process(*stmtPtr);
            }
            // Declarations before any case label are unreachable; ignored.
        }
    } else {
        process(body);
    }
}
} // namespace

void FunctionCodegen::emitSwitchStmt(const wvmcc::parser::SwitchStmt& stmt) {
    using namespace wvmcc::parser;

    std::vector<Segment> segments;
    int defaultSegIdx = -1;
    std::vector<CaseEntry> cases;
    collectSwitchSegments(stmt.body, segments, defaultSegIdx, cases, diagnostics_);

    // Open the outer break-target Block.
    emit(WasmVM::Instr::Block{std::nullopt});
    pushSwitch();
    int breakD = currentBlockDepth_;  // depth after break-block open

    int N = (int)segments.size();
    if (N == 0) {
        // No cases at all: just evaluate the switch expression for side effects.
        emitExpr(stmt.cond);
        emit(WasmVM::Instr::Drop{});
        popControlFlow();
        emit(WasmVM::Instr::End{});
        return;
    }

    // Open one Block per segment, outermost (segment N-1) first so segment 0
    // is innermost and falls through to segment 1, ..., N-1 in source order.
    // Depth assigned to segment i is (breakD + N - i).
    for (int i = N - 1; i >= 0; --i) {
        emit(WasmVM::Instr::Block{std::nullopt});
    }

    // Emit dispatch inside the innermost block.
    // Decide dense vs. sparse using the case-value range vs. count.
    bool dense = !cases.empty();
    long long minV = 0, maxV = 0;
    if (!cases.empty()) {
        minV = cases[0].value;
        maxV = cases[0].value;
        for (const auto& c : cases) {
            if (c.value < minV) minV = c.value;
            if (c.value > maxV) maxV = c.value;
        }
        long long range = maxV - minV;
        long long count = (long long)cases.size();
        // dense if range <= 4 * count and the range fits in a reasonable table
        if (range < 0 || range > (long long)(4 * count) || range > 1024) dense = false;
    }

    int defaultBrIdx;  // Br depth from the innermost block to the default target
    {
        int defaultDepthAtOpen;
        if (defaultSegIdx >= 0) {
            defaultDepthAtOpen = breakD + N - defaultSegIdx;
        } else {
            defaultDepthAtOpen = breakD;  // jump to end of break block
        }
        defaultBrIdx = currentBlockDepth_ - defaultDepthAtOpen;
    }

    auto segBrIdx = [&](int segIdx) {
        int depthAtOpen = breakD + N - segIdx;
        return currentBlockDepth_ - depthAtOpen;
    };

    if (dense) {
        // <expr>; if min != 0: i32.const min; i32.sub; br_table table default
        emitExpr(stmt.cond);
        // Assume i32 switch value (typical for C int). If the expression came
        // back as i64, wrap to i32 for the table dispatch.
        if (getExprType(stmt.cond) == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I32_wrap_i64{});
        }
        if (minV != 0) {
            emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)minV});
            emit(WasmVM::Instr::I32_sub{});
        }
        WasmVM::Instr::Br_table bt;
        long long tableLen = (maxV - minV + 1);
        bt.indices.reserve((size_t)tableLen + 1);
        // Build value→segment lookup for quick mapping
        std::unordered_map<long long, int> valToSeg;
        for (const auto& c : cases) valToSeg[c.value] = c.segmentIdx;
        for (long long v = minV; v <= maxV; ++v) {
            auto it = valToSeg.find(v);
            if (it != valToSeg.end()) {
                bt.indices.push_back((WasmVM::index_t)segBrIdx(it->second));
            } else {
                bt.indices.push_back((WasmVM::index_t)defaultBrIdx);
            }
        }
        bt.indices.push_back((WasmVM::index_t)defaultBrIdx);
        emit(bt);
    } else {
        // Sparse: stash value in a local, chain `local.get; const; eq; br_if`.
        // Use i32 arithmetic; if value is i64, wrap first.
        emitExpr(stmt.cond);
        if (getExprType(stmt.cond) == WasmVM::ValueType::i64) {
            emit(WasmVM::Instr::I32_wrap_i64{});
        }
        int valLocal = allocRawLocal(WasmVM::ValueType::i32);
        emit(WasmVM::Instr::Local_set{(WasmVM::index_t)valLocal});
        for (const auto& c : cases) {
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)valLocal});
            emit(WasmVM::Instr::I32_const{(WasmVM::i32_t)c.value});
            emit(WasmVM::Instr::I32_eq{});
            emit(WasmVM::Instr::Br_if{(WasmVM::index_t)segBrIdx(c.segmentIdx)});
        }
        emit(WasmVM::Instr::Br{(WasmVM::index_t)defaultBrIdx});
    }

    // Close case blocks in source order: after end of segment 0's wrapping
    // block, emit body 0; then end of segment 1's block, body 1; etc.
    for (int i = 0; i < N; ++i) {
        emit(WasmVM::Instr::End{});  // close segment i's block
        for (const auto& s : segments[i].body) {
            emitStmt(s);
        }
    }

    popControlFlow();
    emit(WasmVM::Instr::End{});  // close outer break block
}

WasmVM::ValueType FunctionCodegen::getExprType(const wvmcc::parser::ExprPtr& expr) const {
    // Best-effort: if we can derive a TypeNode, map it. Otherwise fall back to
    // i32 — the historical default that keeps int-only paths working.
    if (auto tn = getExprTypeNode(expr)) {
        return typeMap_.toWasmType(tn);
    }
    return WasmVM::ValueType::i32;
}

wvmcc::parser::TypeNodePtr FunctionCodegen::getExprTypeNode(const wvmcc::parser::ExprPtr& expr) const {
    if (!expr) return nullptr;
    using K = wvmcc::parser::Expr::Kind;

    switch (expr->kind) {
    case K::Float: {
        const auto& fl = static_cast<const wvmcc::parser::FloatLiteral&>(*expr);
        auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        tn->simple.push_back(fl.isFloat
            ? wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Float
            : wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Double);
        return tn;
    }
    case K::Integer: {
        // Must agree with emitIntegerLiteral's width (6.4.4.1): an unsigned
        // literal is 32-bit (i32) up to UINT32_MAX; a signed literal up to
        // INT32_MAX. An L/LL suffix forces the 64-bit (long) form. Disagreement
        // here would make callers insert a spurious i32<->i64 conversion.
        using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
        const auto& il = static_cast<const wvmcc::parser::IntegerLiteral&>(*expr);
        bool isLong = il.isUnsigned
                          ? (std::uint64_t)il.value > 0xFFFFFFFFULL
                          : (il.value > std::numeric_limits<int32_t>::max()
                             || il.value < std::numeric_limits<int32_t>::min());
        for (char c : il.raw) {
            if (c == 'l' || c == 'L') { isLong = true; break; }
        }
        auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        if (il.isUnsigned) tn->simple.push_back(STS::Unsigned);
        tn->simple.push_back(isLong ? STS::Long : STS::Int);
        return tn;
    }
    case K::Char: {
        auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        tn->simple.push_back(
            wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int);
        return tn;
    }
    case K::String: {
        // String literals decay to `char *` (i64 pointer).
        auto charTn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        charTn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        charTn->simple.push_back(
            wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Char);
        auto ptrTn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        ptrTn->kind = wvmcc::parser::TypeNode::Kind::Pointer;
        ptrTn->pointee = charTn;
        return ptrTn;
    }
    case K::Sizeof:
    case K::AlignOf: {
        // Result is size_t — `unsigned long` (i64 on wasm64).
        auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        tn->simple.push_back(
            wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Unsigned);
        tn->simple.push_back(
            wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Long);
        return tn;
    }
    case K::Binary: {
        const auto& b = static_cast<const wvmcc::parser::BinaryExpr&>(*expr);
        // Comma operator (6.5.17): the result has the type/value of the right
        // operand.
        if (b.op == ",") return getExprTypeNode(b.rhs);
        // Comparison and logical ops yield int.
        static const std::unordered_set<std::string> compareOps = {
            "==", "!=", "<", ">", "<=", ">=", "&&", "||"
        };
        if (compareOps.count(b.op)) {
            auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
            tn->simple.push_back(wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int);
            return tn;
        }
        // Plain and compound assignment yield the lhs type.
        if (b.op == "=" || (b.op.size() >= 2 && b.op.back() == '='
                            && b.op != "==" && b.op != "!="
                            && b.op != "<=" && b.op != ">="))
            return getExprTypeNode(b.lhs);
        // Arithmetic / bitwise / shift: usual arithmetic conversions on operand
        // types; here we pick the "wider" of the two.
        auto lt = getExprTypeNode(b.lhs);
        auto rt = getExprTypeNode(b.rhs);
        auto lvt = lt ? typeMap_.toWasmType(lt) : WasmVM::ValueType::i32;
        auto rvt = rt ? typeMap_.toWasmType(rt) : WasmVM::ValueType::i32;
        auto common = arithCommonType(lvt, rvt);
        auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
        switch (common) {
            case WasmVM::ValueType::f64: tn->simple.push_back(STS::Double); break;
            case WasmVM::ValueType::f32: tn->simple.push_back(STS::Float);  break;
            case WasmVM::ValueType::i64: tn->simple.push_back(STS::Long);   break;
            default:                     tn->simple.push_back(STS::Int);    break;
        }
        return tn;
    }
    case K::Cast: {
        const auto& c = static_cast<const wvmcc::parser::CastExpr&>(*expr);
        return c.type;
    }
    case K::GenericSelection: {
        // The result type is that of the selected association's expression.
        const auto& g = static_cast<const wvmcc::parser::GenericSelectionExpr&>(*expr);
        auto chosen = selectGenericAssociation(g);
        return chosen ? getExprTypeNode(chosen) : nullptr;
    }
    case K::Ternary: {
        const auto& t = static_cast<const wvmcc::parser::TernaryExpr&>(*expr);
        // Result is the common-arithmetic type of the two branches; we
        // approximate by returning the "then" branch's type when both are
        // available (matches the common case where both branches yield the
        // same C type).
        auto th = getExprTypeNode(t.thenExpr);
        if (th) return th;
        return getExprTypeNode(t.elseExpr);
    }
    case K::Call: {
        const auto& call = static_cast<const wvmcc::parser::CallExpr&>(*expr);
        if (call.vaArgType) return call.vaArgType; // __builtin_va_arg
        if (call.callee && call.callee->kind == K::Ident) {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*call.callee);
            auto fs = symbolTable_.lookupFunction(id.name);
            if (fs && fs->type) return fs->type;
        }
        return nullptr;
    }
    case K::Ident: {
        const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr);
        // C 6.4.2.2: __func__ has type `const char[]`, decaying to `char *`
        // (an i64 pointer) — matching emitIdentifierExpr's string-pointer emit.
        if (id.name == "__func__" && !symbolTable_.lookup(id.name).has_value()) {
            auto charTn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            charTn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
            charTn->simple.push_back(
                wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Char);
            auto ptrTn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            ptrTn->kind = wvmcc::parser::TypeNode::Kind::Pointer;
            ptrTn->pointee = charTn;
            return ptrTn;
        }
        auto sym = symbolTable_.lookup(id.name);
        if (sym) {
            return std::visit([](const auto& info) -> wvmcc::parser::TypeNodePtr {
                return info.type;
            }, *sym);
        }
        // #79: a bare function name decays to a pointer-to-function value
        // (tagged-i64 funcref-table slot). Synthesize a proper
        // pointer-to-function type (FuncSymbol::type is the return type, stored
        // as the Function node's element) so assignment / initialization sees an
        // i64 and applies no spurious conversion — mirroring the `&func` case.
        if (auto fs = symbolTable_.lookupFunction(id.name)) {
            auto fnTn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            fnTn->kind = wvmcc::parser::TypeNode::Kind::Function;
            fnTn->element = fs->type; // return type (may be null)
            auto ptr = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            ptr->kind = wvmcc::parser::TypeNode::Kind::Pointer;
            ptr->pointee = fnTn;
            return ptr;
        }
        return nullptr;
    }
    case K::Unary: {
        const auto& u = static_cast<const wvmcc::parser::UnaryExpr&>(*expr);
        if (u.op == "*") {
            auto rhsType = getExprTypeNode(u.rhs);
            if (rhsType && rhsType->kind == wvmcc::parser::TypeNode::Kind::Pointer)
                return rhsType->pointee;
        }
        // Arithmetic / bitwise unary ops (+ - ~) preserve the operand type;
        // logical-not yields int.
        if (u.op == "-" || u.op == "+" || u.op == "~"
            || u.op == "++" || u.op == "--") {
            return getExprTypeNode(u.rhs);
        }
        if (u.op == "!") {
            auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
            tn->simple.push_back(
                wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int);
            return tn;
        }
        if (u.op == "&") {
            auto rhsType = getExprTypeNode(u.rhs);
            // M2-L7: `&funcname` decays to a pointer-to-function. With the
            // funcref switch, `toWasmType(Pointer{Function})` is funcref, so
            // synthesize that TypeNode here.
            if (!rhsType && u.rhs && u.rhs->kind == wvmcc::parser::Expr::Kind::Ident) {
                const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*u.rhs);
                if (symbolTable_.lookupFunction(id.name).has_value()) {
                    auto fnTn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    fnTn->kind = wvmcc::parser::TypeNode::Kind::Function;
                    auto ptr = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    ptr->kind = wvmcc::parser::TypeNode::Kind::Pointer;
                    ptr->pointee = fnTn;
                    return ptr;
                }
            }
            if (rhsType) {
                auto ptrNode = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                ptrNode->kind = wvmcc::parser::TypeNode::Kind::Pointer;
                ptrNode->pointee = rhsType;
                return ptrNode;
            }
        }
        return nullptr;
    }
    case K::PostfixUnary: {
        const auto& pu = static_cast<const wvmcc::parser::PostfixUnaryExpr&>(*expr);
        return getExprTypeNode(pu.base); // x++ has the type of x
    }
    case K::Member: {
        const auto& m = static_cast<const wvmcc::parser::MemberExpr&>(*expr);
        auto baseType = getExprTypeNode(m.base);
        if (m.isArrow && baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Pointer)
            baseType = baseType->pointee;
        if (baseType) return typeMap_.getFieldType(baseType, m.member);
        return nullptr;
    }
    case K::Index: {
        const auto& idx = static_cast<const wvmcc::parser::IndexExpr&>(*expr);
        auto baseType = getExprTypeNode(idx.base);
        if (baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Pointer)
            return baseType->pointee;
        if (baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Array)
            return baseType->element;
        return nullptr;
    }
    default:
        return nullptr;
    }
}

} // namespace wvmcc::codegen
