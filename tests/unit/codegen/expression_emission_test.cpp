#include <iostream>
#include "../../../src/codegen/FunctionCodegen.hpp"
#include "../../../src/codegen/TypeMap.hpp"
#include "../../../src/codegen/SymbolTable.hpp"
#include "../../../src/parser/AST.hpp"
#include "instr_check.hpp"

using namespace wvmcc::codegen;
using namespace wvmcc::parser;
using namespace instrcheck;

static int test_emit_integer_literal_i32() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto lit = make_ast<IntegerLiteral>();
    lit->kind = Expr::Kind::Integer;
    lit->value = 42;
    lit->raw = "42";

    codegen.emitExpr(lit);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_emit_integer_literal_i32: expected 1 instruction, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c) {
        std::cerr << "test_emit_integer_literal_i32: expected I32_const\n";
        return 2;
    }
    if (c->value != 42) {
        std::cerr << "test_emit_integer_literal_i32: expected value 42, got " << c->value << "\n";
        return 3;
    }
    return 0;
}

static int test_emit_integer_literal_i64() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto lit = make_ast<IntegerLiteral>();
    lit->kind = Expr::Kind::Integer;
    lit->value = 1LL << 33;
    lit->raw = "8589934592";

    codegen.emitExpr(lit);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_emit_integer_literal_i64: expected 1 instruction, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI64Const(instrs[0]);
    if (!c) {
        std::cerr << "test_emit_integer_literal_i64: expected I64_const\n";
        return 2;
    }
    if (c->value != 1LL << 33) {
        std::cerr << "test_emit_integer_literal_i64: unexpected value\n";
        return 3;
    }
    return 0;
}

static int test_emit_char_literal() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto lit = make_ast<CharLiteral>();
    lit->kind = Expr::Kind::Char;
    lit->value = 'A';

    codegen.emitExpr(lit);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_emit_char_literal: expected 1 instruction, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c) {
        std::cerr << "test_emit_char_literal: expected I32_const\n";
        return 2;
    }
    if (c->value != (int)'A') {
        std::cerr << "test_emit_char_literal: expected value " << (int)'A' << ", got " << c->value << "\n";
        return 3;
    }
    return 0;
}

static int test_emit_binary_add_i32() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto lhs = make_ast<IntegerLiteral>();
    lhs->kind = Expr::Kind::Integer;
    lhs->value = 10;

    auto rhs = make_ast<IntegerLiteral>();
    rhs->kind = Expr::Kind::Integer;
    rhs->value = 20;

    auto bin = make_ast<BinaryExpr>();
    bin->kind = Expr::Kind::Binary;
    bin->op = "+";
    bin->lhs = lhs;
    bin->rhs = rhs;

    codegen.emitExpr(bin);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 3) {
        std::cerr << "test_emit_binary_add_i32: expected 3 instructions, got " << instrs.size() << "\n";
        return 1;
    }
    if (!asI32Const(instrs[0]) || !asI32Const(instrs[1]) ||
        !is(instrs[2], WasmVM::Opcode::I32_add)) {
        std::cerr << "test_emit_binary_add_i32: unexpected instruction sequence\n";
        return 2;
    }
    return 0;
}

// -5  →  [I32_const{5}, I32_const{-1}, I32_xor, I32_const{1}, I32_add]
static int test_emit_unary_negate() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto expr = make_ast<IntegerLiteral>();
    expr->kind = Expr::Kind::Integer;
    expr->value = 5;

    auto unary = make_ast<UnaryExpr>();
    unary->kind = Expr::Kind::Unary;
    unary->op = "-";
    unary->rhs = expr;

    codegen.emitExpr(unary);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 5) {
        std::cerr << "test_emit_unary_negate: expected 5 instructions, got " << instrs.size() << "\n";
        return 1;
    }
    auto i0 = asI32Const(instrs[0]);
    if (!i0 || i0->value != 5)  { std::cerr << "test_emit_unary_negate: [0] expected I32_const{5}\n"; return 2; }
    auto i1 = asI32Const(instrs[1]);
    if (!i1 || i1->value != -1) { std::cerr << "test_emit_unary_negate: [1] expected I32_const{-1}\n"; return 3; }
    if (!is(instrs[2], WasmVM::Opcode::I32_xor)) { std::cerr << "test_emit_unary_negate: [2] expected I32_xor\n"; return 4; }
    auto i3 = asI32Const(instrs[3]);
    if (!i3 || i3->value != 1)  { std::cerr << "test_emit_unary_negate: [3] expected I32_const{1}\n"; return 5; }
    if (!is(instrs[4], WasmVM::Opcode::I32_add)) { std::cerr << "test_emit_unary_negate: [4] expected I32_add\n"; return 6; }
    return 0;
}

// foo()  where foo is FuncSymbol at index 2
// →  [Call{2}]
static int test_call_no_args() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    FuncSymbol sym; sym.type = nullptr; sym.funcIndex = 2; sym.isImport = false;
    symbolTable.defineFunction("foo", sym);

    auto callee = make_ast<IdentifierExpr>();
    callee->kind = Expr::Kind::Ident;
    callee->name = "foo";

    auto callExpr = make_ast<CallExpr>();
    callExpr->kind = Expr::Kind::Call;
    callExpr->callee = callee;

    codegen.emitExpr(callExpr);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_call_no_args: expected 1 instr, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asOneIdx(instrs[0], WasmVM::Opcode::Call);
    if (!c || c->index != 2) {
        std::cerr << "test_call_no_args: expected Call{2}\n";
        return 2;
    }
    return 0;
}

// add(1, 2)  where add is FuncSymbol at index 0
// →  [I32_const{1}, I32_const{2}, Call{0}]
static int test_call_with_args() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    FuncSymbol sym; sym.type = nullptr; sym.funcIndex = 0; sym.isImport = false;
    symbolTable.defineFunction("add", sym);

    auto callee = make_ast<IdentifierExpr>();
    callee->kind = Expr::Kind::Ident;
    callee->name = "add";

    auto arg1 = make_ast<IntegerLiteral>();
    arg1->kind = Expr::Kind::Integer;
    arg1->value = 1;

    auto arg2 = make_ast<IntegerLiteral>();
    arg2->kind = Expr::Kind::Integer;
    arg2->value = 2;

    auto callExpr = make_ast<CallExpr>();
    callExpr->kind = Expr::Kind::Call;
    callExpr->callee = callee;
    callExpr->args.push_back(arg1);
    callExpr->args.push_back(arg2);

    codegen.emitExpr(callExpr);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 3) {
        std::cerr << "test_call_with_args: expected 3 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c1 = asI32Const(instrs[0]);
    if (!c1 || c1->value != 1) { std::cerr << "test_call_with_args: [0] expected I32_const{1}\n"; return 2; }
    auto c2 = asI32Const(instrs[1]);
    if (!c2 || c2->value != 2) { std::cerr << "test_call_with_args: [1] expected I32_const{2}\n"; return 3; }
    auto call = asOneIdx(instrs[2], WasmVM::Opcode::Call);
    if (!call || call->index != 0) { std::cerr << "test_call_with_args: [2] expected Call{0}\n"; return 4; }
    return 0;
}

// puts(0)  where puts is an import FuncSymbol at index 0
// →  [I32_const{0}, Call{0}]
static int test_call_import() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    FuncSymbol sym; sym.type = nullptr; sym.funcIndex = 0; sym.isImport = true;
    symbolTable.defineFunction("puts", sym);

    auto callee = make_ast<IdentifierExpr>();
    callee->kind = Expr::Kind::Ident;
    callee->name = "puts";

    auto arg = make_ast<IntegerLiteral>();
    arg->kind = Expr::Kind::Integer;
    arg->value = 0;

    auto callExpr = make_ast<CallExpr>();
    callExpr->kind = Expr::Kind::Call;
    callExpr->callee = callee;
    callExpr->args.push_back(arg);

    codegen.emitExpr(callExpr);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 2) {
        std::cerr << "test_call_import: expected 2 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 0) { std::cerr << "test_call_import: [0] expected I32_const{0}\n"; return 2; }
    auto call = asOneIdx(instrs[1], WasmVM::Opcode::Call);
    if (!call || call->index != 0) { std::cerr << "test_call_import: [1] expected Call{0}\n"; return 3; }
    return 0;
}

int main() {
    int result;

    result = test_emit_integer_literal_i32();
    if (result != 0) { std::cerr << "test_emit_integer_literal_i32 failed with code " << result << "\n"; return result; }

    result = test_emit_integer_literal_i64();
    if (result != 0) { std::cerr << "test_emit_integer_literal_i64 failed with code " << result << "\n"; return result; }

    result = test_emit_char_literal();
    if (result != 0) { std::cerr << "test_emit_char_literal failed with code " << result << "\n"; return result; }

    result = test_emit_binary_add_i32();
    if (result != 0) { std::cerr << "test_emit_binary_add_i32 failed with code " << result << "\n"; return result; }

    result = test_emit_unary_negate();
    if (result != 0) { std::cerr << "test_emit_unary_negate failed with code " << result << "\n"; return result; }

    result = test_call_no_args();
    if (result != 0) { std::cerr << "test_call_no_args failed with code " << result << "\n"; return result; }

    result = test_call_with_args();
    if (result != 0) { std::cerr << "test_call_with_args failed with code " << result << "\n"; return result; }

    result = test_call_import();
    if (result != 0) { std::cerr << "test_call_import failed with code " << result << "\n"; return result; }

    std::cout << "All expression emission tests passed!" << std::endl;
    return 0;
}
