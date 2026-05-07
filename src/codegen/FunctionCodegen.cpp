#include "FunctionCodegen.hpp"
#include "AddressTakenAnalyzer.hpp"
#include "../parser/ConstExprEval.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <limits>
#include <unordered_map>

namespace wvmcc::codegen {

FunctionCodegen::FunctionCodegen(const TypeMap& typeMap, SymbolTable& symbolTable,
                                 GlobalDataAllocator* dataAllocator)
    : typeMap_(typeMap), symbolTable_(symbolTable), dataAllocator_(dataAllocator) {}

WasmVM::WasmFunc FunctionCodegen::generate(const wvmcc::parser::FunctionDefPtr& funcDef,
                                             const wvmcc::parser::Semantic& semantic) {
    WasmVM::WasmFunc func;

    AddressTakenAnalyzer analyzer;
    addressTakenNames_ = analyzer.analyze(funcDef);

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
    for (const auto& param : funcDef->params) {
        if (param.declarator && !param.declarator->id.name.empty()) {
            // Build TypeNode from param specifiers so getExprTypeNode works for params.
            wvmcc::parser::TypeNodePtr paramType;
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
            // Wrap with pointer if the declarator has a pointer prefix.
            if (paramType && param.declarator->inner.has_value() && *param.declarator->inner
                && (*param.declarator->inner)->kind == wvmcc::parser::Declarator::Kind::Pointer) {
                auto ptrNode = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                ptrNode->kind = wvmcc::parser::TypeNode::Kind::Pointer;
                ptrNode->pointee = paramType;
                paramType = ptrNode;
            }
            ScalarLocal info;
            info.type = paramType;
            info.isAddressTaken = false;
            info.localIndex = paramIdx;
            symbolTable_.define(param.declarator->id.name, info);
        }
        ++paramIdx;
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
    // shadow-stack frame setup:
    //   global.get __stack_pointer   ; push current SP
    //   local.tee  fp_local          ; save as frame pointer, keep on stack
    //   i64.const  frameSize         ; push frame size
    //   i64.sub                      ; new SP = old SP - frameSize
    //   global.set __stack_pointer   ; update SP
    constexpr WasmVM::index_t kStackPtrIdx = 0;
    std::vector<WasmVM::WasmInstr> p;
    p.push_back(WasmVM::Instr::Global_get{kStackPtrIdx});
    p.push_back(WasmVM::Instr::Local_tee{(WasmVM::index_t)framePointerLocal_});
    p.push_back(WasmVM::Instr::I64_const{(WasmVM::i64_t)frameSize_});
    p.push_back(WasmVM::Instr::I64_sub{});
    p.push_back(WasmVM::Instr::Global_set{kStackPtrIdx});
    return p;
}

void FunctionCodegen::generateEpilogue() {
    // shadow-stack frame teardown:
    //   local.get  fp_local          ; restore saved SP
    //   global.set __stack_pointer   ; update SP
    constexpr WasmVM::index_t kStackPtrIdx = 0;
    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
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

void FunctionCodegen::emitIdentifierExpr(const wvmcc::parser::IdentifierExpr& expr, bool needLValue) {
    auto symbolInfo = symbolTable_.lookup(expr.name);
    if (!symbolInfo) {
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
            if (!needLValue) {
                // Load value from shadow-stack memory (mem[1])
                emit(typeMap_.makeLoad(info.type, 1));
            }
        } else if constexpr (std::is_same_v<T, GlobalScalar>) {
            emit(WasmVM::Instr::Global_get{(WasmVM::index_t)info.globalIndex});
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

void FunctionCodegen::emitUnaryExpr(const wvmcc::parser::UnaryExpr& expr, bool needLValue) {
    // Address-of: emit the inner expression as an lvalue (leaves i64 address on stack)
    if (expr.op == "&") {
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
    if (!dataAllocator_) {
        emit(WasmVM::Instr::Unreachable{});
        return;
    }
    size_t addr = dataAllocator_->internString(expr.value);
    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)addr});
}

void FunctionCodegen::emitCallExpr(const wvmcc::parser::CallExpr& expr) {
    // Check if callee returns a struct (needs hidden sret buffer).
    wvmcc::parser::TypeNodePtr calleeRetType;
    int sretBufLocal = -1;

    if (expr.callee && expr.callee->kind == wvmcc::parser::Expr::Kind::Ident) {
        const auto& calleeIdent = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.callee);
        auto funcSym = symbolTable_.lookupFunction(calleeIdent.name);
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

    for (const auto& arg : expr.args) {
        emitExpr(arg);
    }

    if (expr.callee && expr.callee->kind == wvmcc::parser::Expr::Kind::Ident) {
        const auto& calleeExpr = static_cast<const wvmcc::parser::IdentifierExpr&>(*expr.callee);
        auto funcSym = symbolTable_.lookupFunction(calleeExpr.name);
        if (funcSym) {
            emit(WasmVM::Instr::Call{(WasmVM::index_t)funcSym->funcIndex});
            // For struct return: leave the sret buffer address as the "value".
            if (sretBufLocal != -1) {
                emit(WasmVM::Instr::Local_get{(WasmVM::index_t)sretBufLocal});
            }
            return;
        }
    } else {
    }
    emit(WasmVM::Instr::Unreachable{});
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

void FunctionCodegen::emitBlockItem(const wvmcc::parser::BlockItemPtr& item) {
    if (!item) return;

    std::visit([this](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, wvmcc::parser::DeclarationPtr>) {
            if (!v || !v->declarator) return;
            const std::string& name = v->declarator->id.name;

            // Build TypeNode from specifiers
            wvmcc::parser::TypeNodePtr typeNode;
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

            // Wrap with pointer kind if the declarator is a pointer declarator
            if (typeNode && v->declarator->inner.has_value()
                && *v->declarator->inner
                && (*v->declarator->inner)->kind == wvmcc::parser::Declarator::Kind::Pointer) {
                auto ptrNode = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                ptrNode->kind = wvmcc::parser::TypeNode::Kind::Pointer;
                ptrNode->pointee = typeNode;
                typeNode = ptrNode;
            }

            bool isAddrTaken = addressTakenNames_.count(name) > 0;
            // Struct/union variables are always memory-resident
            bool isStructType = typeNode && (typeNode->kind == wvmcc::parser::TypeNode::Kind::Struct
                                           || typeNode->kind == wvmcc::parser::TypeNode::Kind::Union);
            int slotOrOffset = allocLocal(typeNode, isAddrTaken || isStructType);

            if (isAddrTaken || isStructType) {
                MemoryLocal info;
                info.type = typeNode;
                info.frameOffset = (size_t)slotOrOffset;
                symbolTable_.define(name, info);

                // Emit simple expression initializer for MemoryLocal
                if (v->initializer
                    && (*v->initializer)->kind == wvmcc::parser::Initializer::Kind::Expr
                    && (*v->initializer)->expr) {
                    emit(WasmVM::Instr::Local_get{(WasmVM::index_t)framePointerLocal_});
                    emit(WasmVM::Instr::I64_const{(WasmVM::i64_t)info.frameOffset});
                    emit(WasmVM::Instr::I64_add{});
                    emitExpr((*v->initializer)->expr);
                    emit(typeMap_.makeStore(typeNode, 1));
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
    // Placeholder — defaults to i32 until a full type-inference pass exists.
    return WasmVM::ValueType::i32;
}

wvmcc::parser::TypeNodePtr FunctionCodegen::getExprTypeNode(const wvmcc::parser::ExprPtr& expr) const {
    if (!expr) return nullptr;
    using K = wvmcc::parser::Expr::Kind;

    switch (expr->kind) {
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
