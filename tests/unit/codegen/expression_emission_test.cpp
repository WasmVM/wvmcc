#include <iostream>
#include "../../../src/codegen/FunctionCodegen.hpp"
#include "../../../src/codegen/TypeMap.hpp"
#include "../../../src/codegen/SymbolTable.hpp"
#include "../../../src/codegen/GlobalDataAllocator.hpp"
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

// Helper: build an int TypeNode
static wvmcc::parser::TypeNodePtr makeIntType() {
    auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
    node->kind = wvmcc::parser::TypeNode::Kind::Builtin;
    node->simple = {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int};
    return node;
}

// Helper: build a pointer-to-int TypeNode
static wvmcc::parser::TypeNodePtr makeIntPtrType() {
    auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
    node->kind = wvmcc::parser::TypeNode::Kind::Pointer;
    node->pointee = makeIntType();
    return node;
}

// MemoryLocal identifier read (need_value):
//   forceFramePointerLocal(fp=0); define x at frameOffset=8
//   emitExpr(x) →
//     Local_get{0}, I64_const{8}, I64_add, I32_load{mem[1],0,4}
static int test_memory_local_read() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);
    codegen.forceFramePointerLocal(0);  // fp is local 0

    symbolTable.pushScope();
    MemoryLocal ml;
    ml.type = makeIntType();
    ml.frameOffset = 8;
    symbolTable.define("x", ml);

    auto xExpr = make_ast<IdentifierExpr>();
    xExpr->kind = Expr::Kind::Ident;
    xExpr->name = "x";

    codegen.emitExpr(xExpr, false);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    // Expect: Local_get{0}, I64_const{8}, I64_add, I32_load{1,0,4}
    if (instrs.size() != 4) {
        std::cerr << "test_memory_local_read: expected 4 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto lg = asOneIdx(instrs[0], WasmVM::Opcode::Local_get);
    if (!lg || lg->index != 0) { std::cerr << "test_memory_local_read: [0] expected Local_get{0}\n"; return 2; }
    auto ic = asI64Const(instrs[1]);
    if (!ic || ic->value != 8) { std::cerr << "test_memory_local_read: [1] expected I64_const{8}\n"; return 3; }
    if (!is(instrs[2], WasmVM::Opcode::I64_add)) { std::cerr << "test_memory_local_read: [2] expected I64_add\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::I32_load)) { std::cerr << "test_memory_local_read: [3] expected I32_load\n"; return 5; }
    return 0;
}

// MemoryLocal identifier lvalue (need_lvalue):
//   emitExpr(x, needLValue=true) → Local_get{0}, I64_const{8}, I64_add (no load)
static int test_memory_local_lvalue() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);
    codegen.forceFramePointerLocal(0);

    symbolTable.pushScope();
    MemoryLocal ml;
    ml.type = makeIntType();
    ml.frameOffset = 8;
    symbolTable.define("x", ml);

    auto xExpr = make_ast<IdentifierExpr>();
    xExpr->kind = Expr::Kind::Ident;
    xExpr->name = "x";

    codegen.emitExpr(xExpr, true);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 3) {
        std::cerr << "test_memory_local_lvalue: expected 3 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto lg = asOneIdx(instrs[0], WasmVM::Opcode::Local_get);
    if (!lg || lg->index != 0) { std::cerr << "test_memory_local_lvalue: [0] expected Local_get{0}\n"; return 2; }
    auto ic = asI64Const(instrs[1]);
    if (!ic || ic->value != 8) { std::cerr << "test_memory_local_lvalue: [1] expected I64_const{8}\n"; return 3; }
    if (!is(instrs[2], WasmVM::Opcode::I64_add)) { std::cerr << "test_memory_local_lvalue: [2] expected I64_add\n"; return 4; }
    return 0;
}

// &x where x is MemoryLocal → same as lvalue: Local_get{fp}, I64_const{off}, I64_add
static int test_addressof_memory_local() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);
    codegen.forceFramePointerLocal(0);

    symbolTable.pushScope();
    MemoryLocal ml;
    ml.type = makeIntType();
    ml.frameOffset = 0;
    symbolTable.define("x", ml);

    auto xExpr = make_ast<IdentifierExpr>();
    xExpr->kind = Expr::Kind::Ident;
    xExpr->name = "x";

    auto addrOf = make_ast<UnaryExpr>();
    addrOf->kind = Expr::Kind::Unary;
    addrOf->op = "&";
    addrOf->rhs = xExpr;

    codegen.emitExpr(addrOf, false);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    // &x of a shadow-stack local yields a *tagged* pointer value:
    //   Local_get{0}, I64_const{0}, I64_add, I64_const{1<<60}, I64_or
    if (instrs.size() != 5) {
        std::cerr << "test_addressof_memory_local: expected 5 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[2], WasmVM::Opcode::I64_add)) { std::cerr << "test_addressof_memory_local: [2] expected I64_add\n"; return 2; }
    if (!is(instrs[4], WasmVM::Opcode::I64_or))  { std::cerr << "test_addressof_memory_local: [4] expected I64_or (mem[1] tag)\n"; return 3; }
    return 0;
}

// *p where p is ScalarLocal (int* type), needLValue=false:
//   Local_get{p}, I32_load{mem[0], 0, 4}
static int test_deref_pointer() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    ScalarLocal sl;
    sl.type = makeIntPtrType();
    sl.isAddressTaken = false;
    sl.localIndex = 2;
    symbolTable.define("p", sl);

    auto pExpr = make_ast<IdentifierExpr>();
    pExpr->kind = Expr::Kind::Ident;
    pExpr->name = "p";

    auto deref = make_ast<UnaryExpr>();
    deref->kind = Expr::Kind::Unary;
    deref->op = "*";
    deref->rhs = pExpr;

    codegen.emitExpr(deref, false);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    // Local_get{2}, I32_load{0, 0, 4}
    if (instrs.size() != 2) {
        std::cerr << "test_deref_pointer: expected 2 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto lg = asOneIdx(instrs[0], WasmVM::Opcode::Local_get);
    if (!lg || lg->index != 2) { std::cerr << "test_deref_pointer: [0] expected Local_get{2}\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::I32_load)) { std::cerr << "test_deref_pointer: [1] expected I32_load\n"; return 3; }
    return 0;
}

// Pointer arithmetic: p + 1 where p is int* ScalarLocal
// → Local_get{p}, I32_const{1}, I64_extend_i32_s, I64_const{4}, I64_mul, I64_add
static int test_pointer_arithmetic() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    ScalarLocal sl;
    sl.type = makeIntPtrType();
    sl.isAddressTaken = false;
    sl.localIndex = 0;
    symbolTable.define("p", sl);

    auto pExpr = make_ast<IdentifierExpr>();
    pExpr->kind = Expr::Kind::Ident;
    pExpr->name = "p";

    auto one = make_ast<IntegerLiteral>();
    one->kind = Expr::Kind::Integer;
    one->value = 1;

    auto addExpr = make_ast<BinaryExpr>();
    addExpr->kind = Expr::Kind::Binary;
    addExpr->op = "+";
    addExpr->lhs = pExpr;
    addExpr->rhs = one;

    codegen.emitExpr(addExpr, false);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    // Local_get{0}, I32_const{1}, I64_extend_i32_s, I64_const{4}, I64_mul, I64_add
    if (instrs.size() != 6) {
        std::cerr << "test_pointer_arithmetic: expected 6 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Local_get))       { std::cerr << "test_pointer_arithmetic: [0] expected Local_get\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::I32_const))       { std::cerr << "test_pointer_arithmetic: [1] expected I32_const\n"; return 3; }
    if (!is(instrs[2], WasmVM::Opcode::I64_extend_i32_s)){ std::cerr << "test_pointer_arithmetic: [2] expected I64_extend_i32_s\n"; return 4; }
    auto scale = asI64Const(instrs[3]);
    if (!scale || scale->value != 4) { std::cerr << "test_pointer_arithmetic: [3] expected I64_const{4}\n"; return 5; }
    if (!is(instrs[4], WasmVM::Opcode::I64_mul))  { std::cerr << "test_pointer_arithmetic: [4] expected I64_mul\n"; return 6; }
    if (!is(instrs[5], WasmVM::Opcode::I64_add))  { std::cerr << "test_pointer_arithmetic: [5] expected I64_add\n"; return 7; }
    return 0;
}

// ScalarLocal assignment: x = 5 → I32_const{5}, Local_tee{x_local}
static int test_assignment_scalar_local() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    ScalarLocal sl;
    sl.type = makeIntType();
    sl.isAddressTaken = false;
    sl.localIndex = 3;
    symbolTable.define("x", sl);

    auto xExpr = make_ast<IdentifierExpr>();
    xExpr->kind = Expr::Kind::Ident;
    xExpr->name = "x";

    auto five = make_ast<IntegerLiteral>();
    five->kind = Expr::Kind::Integer;
    five->value = 5;

    auto assign = make_ast<BinaryExpr>();
    assign->kind = Expr::Kind::Binary;
    assign->op = "=";
    assign->lhs = xExpr;
    assign->rhs = five;

    codegen.emitExpr(assign, false);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    // I32_const{5}, Local_tee{3}
    if (instrs.size() != 2) {
        std::cerr << "test_assignment_scalar_local: expected 2 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 5) { std::cerr << "test_assignment_scalar_local: [0] expected I32_const{5}\n"; return 2; }
    auto tee = asOneIdx(instrs[1], WasmVM::Opcode::Local_tee);
    if (!tee || tee->index != 3) { std::cerr << "test_assignment_scalar_local: [1] expected Local_tee{3}\n"; return 3; }
    return 0;
}

// String literal with allocator: emits I64_const{addr}
static int test_string_literal() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    wvmcc::codegen::GlobalDataAllocator alloc;
    FunctionCodegen codegen(typeMap, symbolTable, &alloc);

    auto strExpr = make_ast<StringLiteral>();
    strExpr->kind = Expr::Kind::String;
    strExpr->value = "hello";

    codegen.emitExpr(strExpr, false);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_string_literal: expected 1 instr, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI64Const(instrs[0]);
    if (!c) { std::cerr << "test_string_literal: expected I64_const\n"; return 2; }
    if (c->value == 0) { std::cerr << "test_string_literal: address should be non-zero\n"; return 3; }
    return 0;
}

// Member access s.b where s is MemoryLocal struct with field b at offset 4
// (need_value=false): fp+0 address, I64_const{4}, I64_add, I32_load{mem[1],0,4}
static int test_member_access_dot() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);
    codegen.forceFramePointerLocal(0);

    // Build struct type: struct S { int a; int b; }
    auto su = std::make_shared<wvmcc::parser::StructOrUnionSpecifier>();
    su->kind = wvmcc::parser::StructOrUnionSpecifier::Kind::Struct;
    su->hasBody = true;
    su->name = "S";

    // field a: int
    {
        wvmcc::parser::StructMember m;
        wvmcc::parser::DeclarationSpecifiers::TypeSpecifier ts;
        ts.kind = wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
        ts.simple = {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int};
        m.specifiers.typeSpecifiers.push_back(ts);
        wvmcc::parser::StructDeclarator sd;
        sd.declarator = wvmcc::parser::make_ast<wvmcc::parser::Declarator>();
        sd.declarator->kind = wvmcc::parser::Declarator::Kind::Identifier;
        sd.declarator->id.name = "a";
        m.declarators.push_back(sd);
        su->members.push_back(m);
    }
    // field b: int
    {
        wvmcc::parser::StructMember m;
        wvmcc::parser::DeclarationSpecifiers::TypeSpecifier ts;
        ts.kind = wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
        ts.simple = {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int};
        m.specifiers.typeSpecifiers.push_back(ts);
        wvmcc::parser::StructDeclarator sd;
        sd.declarator = wvmcc::parser::make_ast<wvmcc::parser::Declarator>();
        sd.declarator->kind = wvmcc::parser::Declarator::Kind::Identifier;
        sd.declarator->id.name = "b";
        m.declarators.push_back(sd);
        su->members.push_back(m);
    }

    auto structType = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
    structType->kind = wvmcc::parser::TypeNode::Kind::Struct;
    structType->su = su;

    symbolTable.pushScope();
    MemoryLocal ml;
    ml.type = structType;
    ml.frameOffset = 0;
    symbolTable.define("s", ml);

    auto sExpr = make_ast<IdentifierExpr>();
    sExpr->kind = Expr::Kind::Ident;
    sExpr->name = "s";

    auto memberExpr = make_ast<wvmcc::parser::MemberExpr>();
    memberExpr->kind = Expr::Kind::Member;
    memberExpr->base = sExpr;
    memberExpr->member = "b";
    memberExpr->isArrow = false;

    codegen.emitExpr(memberExpr, false);
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    // Expect: Local_get{0}, I64_const{0}, I64_add (base address),
    //         I64_const{4}, I64_add (field offset),
    //         I32_load{1,0,4}
    if (instrs.size() != 6) {
        std::cerr << "test_member_access_dot: expected 6 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Local_get)) { std::cerr << "test_member_access_dot: [0] expected Local_get\n"; return 2; }
    // instrs[1] = I64_const{0}, instrs[2] = I64_add  (base lvalue)
    if (!is(instrs[2], WasmVM::Opcode::I64_add)) { std::cerr << "test_member_access_dot: [2] expected I64_add\n"; return 3; }
    auto fieldOff = asI64Const(instrs[3]);
    if (!fieldOff || fieldOff->value != 4) { std::cerr << "test_member_access_dot: [3] expected I64_const{4}\n"; return 4; }
    if (!is(instrs[4], WasmVM::Opcode::I64_add)) { std::cerr << "test_member_access_dot: [4] expected I64_add\n"; return 5; }
    if (!is(instrs[5], WasmVM::Opcode::I32_load)) { std::cerr << "test_member_access_dot: [5] expected I32_load\n"; return 6; }
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

    result = test_memory_local_read();
    if (result != 0) { std::cerr << "test_memory_local_read failed with code " << result << "\n"; return result; }

    result = test_memory_local_lvalue();
    if (result != 0) { std::cerr << "test_memory_local_lvalue failed with code " << result << "\n"; return result; }

    result = test_addressof_memory_local();
    if (result != 0) { std::cerr << "test_addressof_memory_local failed with code " << result << "\n"; return result; }

    result = test_deref_pointer();
    if (result != 0) { std::cerr << "test_deref_pointer failed with code " << result << "\n"; return result; }

    result = test_pointer_arithmetic();
    if (result != 0) { std::cerr << "test_pointer_arithmetic failed with code " << result << "\n"; return result; }

    result = test_assignment_scalar_local();
    if (result != 0) { std::cerr << "test_assignment_scalar_local failed with code " << result << "\n"; return result; }

    result = test_string_literal();
    if (result != 0) { std::cerr << "test_string_literal failed with code " << result << "\n"; return result; }

    result = test_member_access_dot();
    if (result != 0) { std::cerr << "test_member_access_dot failed with code " << result << "\n"; return result; }

    std::cout << "All expression emission tests passed!" << std::endl;
    return 0;
}
