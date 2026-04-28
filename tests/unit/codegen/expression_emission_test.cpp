#include <iostream>
#include "../../../src/codegen/FunctionCodegen.hpp"
#include "../../../src/codegen/TypeMap.hpp"
#include "../../../src/codegen/SymbolTable.hpp"
#include "../../../src/parser/AST.hpp"

using namespace wvmcc::codegen;
using namespace wvmcc::parser;

static int test_emit_integer_literal_i32() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto lit = make_ast<IntegerLiteral>();
    lit->value = 42;
    lit->raw = "42";

    codegen.emitExpr(lit);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_emit_integer_literal_i32: expected 1 instruction, got " << instrs.size() << "\n";
        return 1;
    }
    auto* instr = std::get_if<WasmVM::Instr::I32_const>(&instrs[0]);
    if (!instr) {
        std::cerr << "test_emit_integer_literal_i32: expected I32_const\n";
        return 2;
    }
    if (instr->value != 42) {
        std::cerr << "test_emit_integer_literal_i32: expected value 42, got " << instr->value << "\n";
        return 3;
    }
    return 0;
}

static int test_emit_integer_literal_i64() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto lit = make_ast<IntegerLiteral>();
    lit->value = 1LL << 33;
    lit->raw = "8589934592";

    codegen.emitExpr(lit);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_emit_integer_literal_i64: expected 1 instruction, got " << instrs.size() << "\n";
        return 1;
    }
    auto* instr = std::get_if<WasmVM::Instr::I64_const>(&instrs[0]);
    if (!instr) {
        std::cerr << "test_emit_integer_literal_i64: expected I64_const\n";
        return 2;
    }
    if (instr->value != 1LL << 33) {
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
    lit->value = 'A';

    codegen.emitExpr(lit);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_emit_char_literal: expected 1 instruction, got " << instrs.size() << "\n";
        return 1;
    }
    auto* instr = std::get_if<WasmVM::Instr::I32_const>(&instrs[0]);
    if (!instr) {
        std::cerr << "test_emit_char_literal: expected I32_const\n";
        return 2;
    }
    if (instr->value != (int)'A') {
        std::cerr << "test_emit_char_literal: expected value " << (int)'A' << ", got " << instr->value << "\n";
        return 3;
    }
    return 0;
}

static int test_emit_binary_add_i32() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto lhs = make_ast<IntegerLiteral>();
    lhs->value = 10;

    auto rhs = make_ast<IntegerLiteral>();
    rhs->value = 20;

    auto bin = make_ast<BinaryExpr>();
    bin->op = "+";
    bin->lhs = lhs;
    bin->rhs = rhs;

    codegen.emitExpr(bin);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 3) {
        std::cerr << "test_emit_binary_add_i32: expected 3 instructions, got " << instrs.size() << "\n";
        return 1;
    }
    if (!std::get_if<WasmVM::Instr::I32_const>(&instrs[0]) ||
        !std::get_if<WasmVM::Instr::I32_const>(&instrs[1]) ||
        !std::get_if<WasmVM::Instr::I32_add>(&instrs[2])) {
        std::cerr << "test_emit_binary_add_i32: unexpected instruction sequence\n";
        return 2;
    }
    return 0;
}

static int test_emit_unary_negate() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto expr = make_ast<IntegerLiteral>();
    expr->value = 5;

    auto unary = make_ast<UnaryExpr>();
    unary->op = "-";
    unary->rhs = expr;

    codegen.emitExpr(unary);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 2) {
        std::cerr << "test_emit_unary_negate: expected 2 instructions, got " << instrs.size() << "\n";
        return 1;
    }
    auto* i1 = std::get_if<WasmVM::Instr::I32_const>(&instrs[0]);
    if (!i1 || i1->value != 5) {
        std::cerr << "test_emit_unary_negate: unexpected first instruction\n";
        return 2;
    }
    if (!std::get_if<WasmVM::Instr::I32_sub>(&instrs[1])) {
        std::cerr << "test_emit_unary_negate: expected I32_sub\n";
        return 3;
    }
    return 0;
}

int main() {
    int result;

    result = test_emit_integer_literal_i32();
    if (result != 0) {
        std::cerr << "test_emit_integer_literal_i32 failed with code " << result << "\n";
        return result;
    }

    result = test_emit_integer_literal_i64();
    if (result != 0) {
        std::cerr << "test_emit_integer_literal_i64 failed with code " << result << "\n";
        return result;
    }

    result = test_emit_char_literal();
    if (result != 0) {
        std::cerr << "test_emit_char_literal failed with code " << result << "\n";
        return result;
    }

    result = test_emit_binary_add_i32();
    if (result != 0) {
        std::cerr << "test_emit_binary_add_i32 failed with code " << result << "\n";
        return result;
    }

    result = test_emit_unary_negate();
    if (result != 0) {
        std::cerr << "test_emit_unary_negate failed with code " << result << "\n";
        return result;
    }

    std::cout << "All expression emission tests passed!" << std::endl;
    return 0;
}
