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
    fc->emit(WasmVM::Instr::Unreachable{});
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
                    paramType = semantic_->buildTypeFromDeclaration(param.specifiers, param.declarator);
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
        prologue.insert(prologue.end(), instrBuffer_.begin(), instrBuffer_.end());
        instrBuffer_ = std::move(prologue);
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
        if (!moduleCg_) { emit(WasmVM::Instr::Unreachable{}); break; }

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
        // M2-L7: bare function name in value context decays to a funcref via
        // ref.func, so the linker can renumber function indices without
        // rewriting hardcoded slot constants in user code.
        if (!needLValue) {
            auto funcSym = symbolTable_.lookupFunction(expr.name);
            if (funcSym) {
                emit(WasmVM::Instr::Ref_func{(WasmVM::index_t)funcSym->funcIndex});
                return;
            }
        }
        emit(WasmVM::Instr::Unreachable{});
        return;
    }

    std::visit([this, needLValue](const auto& info) {
        using T = std::decay_t<decltype(info)>;
        if constexpr (std::is_same_v<T, ScalarLocal>) {
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)info.localIndex});
        } else if constexpr (std::is_same_v<T, MemoryLocal>) {
            // Compute shadow-stack address: fp + frameOffset
            emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.frameOffset});
            emit(WasmVM::Instr::I64_add{});
            // Arrays decay to pointers in expression context, so leave the
            // base address on the stack rather than loading.
            bool isArray = info.type
                && info.type->kind == wvmcc::parser::TypeNode::Kind::Array;
            if (!needLValue && !isArray) {
                emit(typeMap_.makeLoad(info.type, 1));
            }
        } else if constexpr (std::is_same_v<T, GlobalScalar>) {
            emit(WasmVM::Instr::Global_get{(WasmVM::index_t)info.globalIndex});
        } else if constexpr (std::is_same_v<T, GlobalMem>) {
            // Static local: address is in mem[0].
            emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.address});
            if (!needLValue) {
                emit(typeMap_.makeLoad(info.type, 0));
            }
        } else {
            emit(WasmVM::Instr::Unreachable{});
        }
    }, *symbolInfo);
}

void FunctionCodegen::emitBinaryExpr(const wvmcc::parser::BinaryExpr& expr) {
    using K = wvmcc::parser::Expr::Kind;

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
                    // Static local: address is in mem[0].
                    auto rhsWasmType = getExprType(expr.rhs);
                    int tempIdx = allocRawLocal(rhsWasmType);
                    emitExpr(expr.rhs, false);
                    emit(WasmVM::Instr::Local_set{(WasmVM::index_t)tempIdx});
                    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)gm->address});
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
                    emit(typeMap_.makeStore(gm->type, 0));
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});
                    return;
                }
            }
        }

        // General lvalue assignment (pointer dereference, member access, array index)
        auto rhsWasmType = getExprType(expr.rhs);
        int tempIdx = allocRawLocal(rhsWasmType);
        emitExpr(expr.rhs, false);
        emit(WasmVM::Instr::Local_set{(WasmVM::index_t)tempIdx});

        emitExpr(expr.lhs, true);  // push lhs address
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)tempIdx});

        bool useHeap = false;
        if (expr.lhs) {
            if (expr.lhs->kind == K::Unary) {
                const auto& u = static_cast<const wvmcc::parser::UnaryExpr&>(*expr.lhs);
                useHeap = (u.op == "*");
            } else if (expr.lhs->kind == K::Member) {
                const auto& m = static_cast<const wvmcc::parser::MemberExpr&>(*expr.lhs);
                useHeap = m.isArrow;
            } else if (expr.lhs->kind == K::Index) {
                useHeap = true;
            }
        }
        auto lhsTypeNode = getExprTypeNode(expr.lhs);
        emit(typeMap_.makeStore(lhsTypeNode, useHeap ? 0 : 1));
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
    auto lhsTypeNode = getExprTypeNode(expr.lhs);
    bool lhsIsPointer = lhsTypeNode && lhsTypeNode->kind == wvmcc::parser::TypeNode::Kind::Pointer;

    if (lhsIsPointer && (expr.op == "+" || expr.op == "-")) {
        emitExpr(expr.lhs, false);
        emitExpr(expr.rhs, false);
        auto rhsWasmType = getExprType(expr.rhs);
        if (rhsWasmType == WasmVM::ValueType::i32) {
            emit(WasmVM::Instr::I64_extend_i32_s{});
        }
        size_t pointeeSize = lhsTypeNode->pointee ? typeMap_.byteSize(lhsTypeNode->pointee) : 1;
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
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "-") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_sub{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_sub{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_sub{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_sub{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "*") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_mul{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_mul{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_mul{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_mul{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "/") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_div_s{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_div_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_div{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_div{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "%") {
        // C: % is integer-only; floats are a constraint violation.
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_rem_s{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_rem_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "==") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_eq{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_eq{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_eq{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_eq{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "!=") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_ne{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_ne{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_ne{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_ne{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "<") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_lt_s{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_lt_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_lt{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_lt{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == ">") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_gt_s{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_gt_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_gt{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_gt{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "<=") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_le_s{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_le_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_le{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_le{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == ">=") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_ge_s{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_ge_s{});
        else if (commonType == VT::f32) emit(WasmVM::Instr::F32_ge{});
        else if (commonType == VT::f64) emit(WasmVM::Instr::F64_ge{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "&") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_and{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_and{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "|") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_or{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_or{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "^") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_xor{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_xor{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == "<<") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_shl{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_shl{});
        else emit(WasmVM::Instr::Unreachable{});
    } else if (expr.op == ">>") {
        if (commonType == VT::i32) emit(WasmVM::Instr::I32_shr_s{});
        else if (commonType == VT::i64) emit(WasmVM::Instr::I64_shr_s{});
        else emit(WasmVM::Instr::Unreachable{});
    } else {
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr, bool needLValue) {
    // Address-of: emit the inner expression as an lvalue (leaves i64 address on stack)
    if (expr.op == "&") {
        // M2-L7: &funcname produces a funcref via ref.func. The linker
        // remaps the funcidx (no hardcoded slot indices in user code).
        if (expr.rhs && expr.rhs->kind == wvmcc::parser::Expr::Kind::Ident) {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.rhs);
            auto funcSym = symbolTable_.lookupFunction(id.name);
            if (funcSym) {
                emit(WasmVM::Instr::Ref_func{(WasmVM::index_t)funcSym->funcIndex});
                return;
            }
        }
        emitExpr(expr.rhs, true);
        return;
    }

    // Dereference: emit the pointer value; load pointee from mem[0] if need_value
    if (expr.op == "*") {
        emitExpr(expr.rhs, false);
        if (!needLValue) {
            auto rhsTypeNode = getExprTypeNode(expr.rhs);
            wvmcc::parser::TypeNodePtr pointeeType;
            if (rhsTypeNode && rhsTypeNode->kind == wvmcc::parser::TypeNode::Kind::Pointer)
                pointeeType = rhsTypeNode->pointee;
            emit(typeMap_.makeLoad(pointeeType, 0));
        }
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
        emit(WasmVM::Instr::Unreachable{});
    }
}

void FunctionCodegen::emitStringLiteral(const wvmcc::parser::StringLiteral& expr) {
    if (!dataAllocator_) {
        emit(WasmVM::Instr::Unreachable{});
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

    if (isDirect) {
        auto funcSym = symbolTable_.lookupFunction(directName);
        if (funcSym && funcSym->type
            && (funcSym->type->kind == wvmcc::parser::TypeNode::Kind::Struct
                || funcSym->type->kind == wvmcc::parser::TypeNode::Kind::Union)) {
            calleeRetType = funcSym->type;
        }
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
        // M2-L7: function pointers are funcref values. Push the funcref and
        // use call_ref <typeidx>; no table indirection is needed.
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
            for (const auto& p : fnNode->params) {
                ft.params.push_back(typeMap_.toWasmType(p));
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

        emit(WasmVM::Instr::Call_ref{*typeIdx});
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
                auto srcVt = getExprType(expr.args[i]);
                auto dstVt = paramTypes[i];
                if (srcVt != dstVt) {
                    if (srcVt == WasmVM::ValueType::i32
                        && dstVt == WasmVM::ValueType::i64) {
                        emit(WasmVM::Instr::I64_extend_i32_s{});
                    } else if (srcVt == WasmVM::ValueType::i64
                               && dstVt == WasmVM::ValueType::i32) {
                        emit(WasmVM::Instr::I32_wrap_i64{});
                    }
                    // Other coercions (int↔float etc.) intentionally
                    // omitted — they don't show up at libc call sites yet.
                }
            }
        }
        if (isDirect) {
            emitDirectCall();
        } else {
            emitIndirectCall();
        }
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

    // 7. Restore SP from saved value if we changed it.
    if (numVariadic > 0) {
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)savedSpLocal});
        emit(WasmVM::Instr::Global_set{kStackPtrIdx});
    }

    if (sretBufLocal != -1) {
        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)sretBufLocal});
    }
}

void FunctionCodegen::emitMemberAccessExpr(const wvmcc::parser::MemberExpr& expr, bool needLValue) {
    // Determine base struct type for field-offset lookup
    auto baseType = getExprTypeNode(expr.base);
    if (expr.isArrow && baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Pointer) {
        baseType = baseType->pointee;
    }

    size_t fieldOffset = baseType ? typeMap_.getFieldOffset(baseType, expr.member) : 0;
    auto fieldType     = baseType ? typeMap_.getFieldType(baseType, expr.member)   : nullptr;
    // . accesses shadow-stack locals (mem[1]); -> accesses heap objects (mem[0])
    uint8_t memidx = expr.isArrow ? 0 : 1;

    if (expr.isArrow) {
        emitExpr(expr.base, false);  // pointer value is the base address
    } else {
        emitExpr(expr.base, true);   // lvalue address of the struct
    }

    if (fieldOffset > 0) {
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)fieldOffset});
        emit(WasmVM::Instr::I64_add{});
    }

    if (!needLValue) {
        emit(typeMap_.makeLoad(fieldType, memidx));
    }
}

void FunctionCodegen::emitArrayIndexExpr(const wvmcc::parser::IndexExpr& expr, bool needLValue) {
    // Equivalent to *(base + index * sizeof(*base))
    auto baseType = getExprTypeNode(expr.base);

    wvmcc::parser::TypeNodePtr elemType;
    bool baseIsPointer = false;
    if (baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Pointer) {
        elemType = baseType->pointee;
        baseIsPointer = true;
    } else if (baseType && baseType->kind == wvmcc::parser::TypeNode::Kind::Array) {
        elemType = baseType->element;
    }

    size_t elemSize = elemType ? typeMap_.byteSize(elemType) : 4;
    // Heap pointers use mem[0]; shadow-stack arrays use mem[1]
    uint8_t memidx = baseIsPointer ? 0 : 1;

    emitExpr(expr.base, false);    // base address (i64)
    emitExpr(expr.index, false);   // index

    auto idxWasmType = getExprType(expr.index);
    if (idxWasmType == WasmVM::ValueType::i32) {
        emit(WasmVM::Instr::I64_extend_i32_s{});
    }

    if (elemSize > 1) {
        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)elemSize});
        emit(WasmVM::Instr::I64_mul{});
    }

    emit(WasmVM::Instr::I64_add{});

    if (!needLValue) {
        emit(typeMap_.makeLoad(elemType, memidx));
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
        emit(WasmVM::Instr::Unreachable{});
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
        emit(WasmVM::Instr::Unreachable{});
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

            // Prefer Semantic::buildTypeFromDeclaration (handles pointer-to-function,
            // arrays, etc.). Fall back to a hand-built TypeNode for tests that don't
            // wire up a Semantic instance.
            wvmcc::parser::TypeNodePtr typeNode;
            if (semantic_) {
                typeNode = semantic_->buildTypeFromDeclaration(v->specifiers, v->declarator);
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
                        emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
                        emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.frameOffset});
                        emit(WasmVM::Instr::I64_add{});
                        emitExpr((*v->initializer)->expr);
                        emit(typeMap_.makeStore(typeNode, 1));
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
    }
    if (framePointerLocal_ != -1 && frameSize_ > 0) {
        generateEpilogue();
    }
    emit(WasmVM::Instr::Return{});
}

void FunctionCodegen::emitStructCopyToHiddenPtr(const wvmcc::parser::ExprPtr& srcExpr) {
    if (!returnTypeNode_ || !returnTypeNode_->su) {
        // No field info: emit unreachable placeholder.
        emit(WasmVM::Instr::Unreachable{});
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

void FunctionCodegen::emitExprStmt(const wvmcc::parser::ExprStmt& stmt) {
    if (!stmt.expr) return;

    // Determine whether the expression leaves a value on the stack.
    // For direct calls to known functions, check the callee's return type.
    bool leavesValue = true;
    if (stmt.expr->kind == wvmcc::parser::Expr::Kind::Call) {
        const auto& call = static_cast<const wvmcc::parser::CallExpr&>(*stmt.expr);
        if (call.callee && call.callee->kind == wvmcc::parser::Expr::Kind::Ident) {
            const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*call.callee);
            // __builtin_va_start/end/copy emit no value; __builtin_va_arg does.
            if (id.name == "__builtin_va_start"
                || id.name == "__builtin_va_end"
                || id.name == "__builtin_va_copy") {
                leavesValue = false;
            } else if (id.name == "__builtin_va_arg") {
                leavesValue = true;
            } else {
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
                    leavesValue = !isVoidRet && !isStructRet;
                }
            }
        }
    }

    emitExpr(stmt.expr);
    if (leavesValue) {
        emit(WasmVM::Instr::Drop{});
    }
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
    //   loop $continue
    //     <cond>; i32.eqz; br_if $break
    //     <body>
    //     <step>
    //     br $continue
    //   end
    // end
    // Note: `continue` re-enters the loop top, skipping any remaining body
    // statements but also skipping the step. Strict C semantics would require
    // a nested $continue block above step; this simpler form matches the
    // existing tests and the Phase 3 issue specification.
    symbolTable_.pushScope();
    if (stmt.init) {
        emitBlockItem(*stmt.init);
    }
    emit(WasmVM::Instr::Block{std::nullopt});
    int breakD = currentBlockDepth_;
    emit(WasmVM::Instr::Loop{std::nullopt});
    int contD = currentBlockDepth_;
    pushLoop(breakD, contD);
    if (stmt.cond) {
        emitExpr(*stmt.cond);
        emit(WasmVM::Instr::I32_eqz{});
        emit(WasmVM::Instr::Br_if{breakDepth()});
    }
    emitStmt(stmt.body);
    if (stmt.step) {
        emitExpr(*stmt.step);
        emit(WasmVM::Instr::Drop{});
    }
    emit(WasmVM::Instr::Br{continueDepth()});
    popControlFlow();
    emit(WasmVM::Instr::End{});
    emit(WasmVM::Instr::End{});
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
    case K::Integer:
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
    case K::Binary: {
        const auto& b = static_cast<const wvmcc::parser::BinaryExpr&>(*expr);
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
        // Assignment yields the lhs type.
        if (b.op == "=") return getExprTypeNode(b.lhs);
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
        auto sym = symbolTable_.lookup(id.name);
        if (!sym) return nullptr;
        return std::visit([](const auto& info) -> wvmcc::parser::TypeNodePtr {
            return info.type;
        }, *sym);
    }
    case K::Unary: {
        const auto& u = static_cast<const wvmcc::parser::UnaryExpr&>(*expr);
        if (u.op == "*") {
            auto rhsType = getExprTypeNode(u.rhs);
            if (rhsType && rhsType->kind == wvmcc::parser::TypeNode::Kind::Pointer)
                return rhsType->pointee;
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
