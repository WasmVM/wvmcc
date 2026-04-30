#include <iostream>
#include "../../../src/codegen/FunctionCodegen.hpp"
#include "../../../src/codegen/TypeMap.hpp"
#include "../../../src/codegen/SymbolTable.hpp"
#include "../../../src/parser/AST.hpp"
#include "../../../src/parser/Semantic.hpp"
#include "instr_check.hpp"

using namespace wvmcc::codegen;
using namespace wvmcc::parser;
using namespace instrcheck;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ExprPtr makeI32(int32_t v) {
    auto lit = make_ast<IntegerLiteral>();
    lit->kind = Expr::Kind::Integer;
    lit->value = v;
    return lit;
}

static StmtPtr makeReturnStmt(ExprPtr val = nullptr) {
    auto s = make_ast<ReturnStmt>();
    s->kind = Stmt::Kind::Return;
    if (val) s->value = val;
    return s;
}

static StmtPtr makeExprStmt(ExprPtr expr) {
    auto s = make_ast<ExprStmt>();
    s->kind = Stmt::Kind::Expr;
    s->expr = expr;
    return s;
}

static BlockItemPtr wrapStmt(StmtPtr stmt) {
    auto item = make_ast<BlockItem>();
    item->item = stmt;
    return item;
}

static BlockItemPtr wrapDecl(DeclarationPtr decl) {
    auto item = make_ast<BlockItem>();
    item->item = decl;
    return item;
}

static DeclarationPtr makeIntDecl(const std::string& name, ExprPtr initExpr = nullptr) {
    auto decl = make_ast<Declaration>();
    decl->declarator = make_ast<Declarator>();
    decl->declarator->kind = Declarator::Kind::Identifier;
    decl->declarator->id.name = name;

    DeclarationSpecifiers::TypeSpecifier ts;
    ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
    ts.simple = {DeclarationSpecifiers::SimpleTypeSpecifier::Int};
    decl->specifiers.typeSpecifiers.push_back(ts);

    if (initExpr) {
        auto init = make_ast<Initializer>();
        init->kind = Initializer::Kind::Expr;
        init->expr = initExpr;
        decl->initializer = init;
    }
    return decl;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// return;  →  [Return]
static int test_return_void() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    codegen.emitStmt(makeReturnStmt());

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 1) {
        std::cerr << "test_return_void: expected 1 instr, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Return)) {
        std::cerr << "test_return_void: expected Return\n";
        return 2;
    }
    return 0;
}

// return 42;  →  [I32_const{42}, Return]
static int test_return_value() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    codegen.emitStmt(makeReturnStmt(makeI32(42)));

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 2) {
        std::cerr << "test_return_value: expected 2 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 42) {
        std::cerr << "test_return_value: expected I32_const{42}\n";
        return 2;
    }
    if (!is(instrs[1], WasmVM::Opcode::Return)) {
        std::cerr << "test_return_value: expected Return\n";
        return 3;
    }
    return 0;
}

// 5;  →  [I32_const{5}, Drop]
static int test_expr_stmt() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    codegen.emitStmt(makeExprStmt(makeI32(5)));

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 2) {
        std::cerr << "test_expr_stmt: expected 2 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 5) {
        std::cerr << "test_expr_stmt: expected I32_const{5}\n";
        return 2;
    }
    if (!is(instrs[1], WasmVM::Opcode::Drop)) {
        std::cerr << "test_expr_stmt: expected Drop\n";
        return 3;
    }
    return 0;
}

// ;  →  [] (no instructions)
static int test_empty_stmt() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto s = make_ast<Stmt>();
    s->kind = Stmt::Kind::Empty;
    codegen.emitStmt(s);

    const auto& instrs = codegen.getInstructions();
    if (!instrs.empty()) {
        std::cerr << "test_empty_stmt: expected no instrs, got " << instrs.size() << "\n";
        return 1;
    }
    return 0;
}

// { 1; 2; }  →  [I32_const{1}, Drop, I32_const{2}, Drop]
static int test_compound_stmt() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto cs = make_ast<CompoundStmt>();
    cs->kind = Stmt::Kind::Compound;
    cs->items.push_back(wrapStmt(makeExprStmt(makeI32(1))));
    cs->items.push_back(wrapStmt(makeExprStmt(makeI32(2))));

    codegen.emitStmt(cs);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 4) {
        std::cerr << "test_compound_stmt: expected 4 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c1 = asI32Const(instrs[0]);
    if (!c1 || c1->value != 1) { std::cerr << "test_compound_stmt: [0] bad\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::Drop)) { std::cerr << "test_compound_stmt: [1] bad\n"; return 3; }
    auto c2 = asI32Const(instrs[2]);
    if (!c2 || c2->value != 2) { std::cerr << "test_compound_stmt: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Drop)) { std::cerr << "test_compound_stmt: [3] bad\n"; return 5; }
    return 0;
}

// if (1) return 2;  →  [I32_const{1}, If, I32_const{2}, Return, End]
static int test_if_no_else() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto is_ = make_ast<IfStmt>();
    is_->kind = Stmt::Kind::If;
    is_->cond = makeI32(1);
    is_->thenStmt = makeReturnStmt(makeI32(2));

    codegen.emitStmt(is_);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 5) {
        std::cerr << "test_if_no_else: expected 5 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!asI32Const(instrs[0]))                          { std::cerr << "test_if_no_else: [0] bad\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::If))              { std::cerr << "test_if_no_else: [1] bad\n"; return 3; }
    if (!asI32Const(instrs[2]))                          { std::cerr << "test_if_no_else: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Return))          { std::cerr << "test_if_no_else: [3] bad\n"; return 5; }
    if (!is(instrs[4], WasmVM::Opcode::End))             { std::cerr << "test_if_no_else: [4] bad\n"; return 6; }
    return 0;
}

// if (1) return 2; else return 3;
// →  [I32_const{1}, If, I32_const{2}, Return, Else, I32_const{3}, Return, End]
static int test_if_with_else() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto is_ = make_ast<IfStmt>();
    is_->kind = Stmt::Kind::If;
    is_->cond = makeI32(1);
    is_->thenStmt = makeReturnStmt(makeI32(2));
    is_->elseStmt = makeReturnStmt(makeI32(3));

    codegen.emitStmt(is_);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 8) {
        std::cerr << "test_if_with_else: expected 8 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!asI32Const(instrs[0]))                          { std::cerr << "test_if_with_else: [0] bad\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::If))              { std::cerr << "test_if_with_else: [1] bad\n"; return 3; }
    if (!asI32Const(instrs[2]))                          { std::cerr << "test_if_with_else: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Return))          { std::cerr << "test_if_with_else: [3] bad\n"; return 5; }
    if (!is(instrs[4], WasmVM::Opcode::Else))            { std::cerr << "test_if_with_else: [4] bad\n"; return 6; }
    if (!asI32Const(instrs[5]))                          { std::cerr << "test_if_with_else: [5] bad\n"; return 7; }
    if (!is(instrs[6], WasmVM::Opcode::Return))          { std::cerr << "test_if_with_else: [6] bad\n"; return 8; }
    if (!is(instrs[7], WasmVM::Opcode::End))             { std::cerr << "test_if_with_else: [7] bad\n"; return 9; }
    return 0;
}

// while (1) return 0;
// →  [Block, Loop, I32_const{1}, I32_eqz, Br_if{1}, I32_const{0}, Return, Br{0}, End, End]
static int test_while_stmt() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto ws = make_ast<WhileStmt>();
    ws->kind = Stmt::Kind::While;
    ws->cond = makeI32(1);
    ws->body = makeReturnStmt(makeI32(0));

    codegen.emitStmt(ws);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 10) {
        std::cerr << "test_while_stmt: expected 10 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Block))           { std::cerr << "test_while_stmt: [0] bad\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::Loop))            { std::cerr << "test_while_stmt: [1] bad\n"; return 3; }
    if (!asI32Const(instrs[2]))                          { std::cerr << "test_while_stmt: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::I32_eqz))         { std::cerr << "test_while_stmt: [3] bad\n"; return 5; }
    auto brif = asOneIdx(instrs[4], WasmVM::Opcode::Br_if);
    if (!brif || brif->index != 1)                       { std::cerr << "test_while_stmt: [4] bad\n"; return 6; }
    if (!asI32Const(instrs[5]))                          { std::cerr << "test_while_stmt: [5] bad\n"; return 7; }
    if (!is(instrs[6], WasmVM::Opcode::Return))          { std::cerr << "test_while_stmt: [6] bad\n"; return 8; }
    auto br = asOneIdx(instrs[7], WasmVM::Opcode::Br);
    if (!br || br->index != 0)                           { std::cerr << "test_while_stmt: [7] bad\n"; return 9; }
    if (!is(instrs[8], WasmVM::Opcode::End))             { std::cerr << "test_while_stmt: [8] bad\n"; return 10; }
    if (!is(instrs[9], WasmVM::Opcode::End))             { std::cerr << "test_while_stmt: [9] bad\n"; return 11; }
    return 0;
}

// for (;;) return 0;
// →  [Block, Loop, I32_const{0}, Return, Br{0}, End, End]
static int test_for_no_cond() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto fs = make_ast<ForStmt>();
    fs->kind = Stmt::Kind::For;
    fs->body = makeReturnStmt(makeI32(0));

    codegen.emitStmt(fs);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 7) {
        std::cerr << "test_for_no_cond: expected 7 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Block))   { std::cerr << "test_for_no_cond: [0] bad\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::Loop))    { std::cerr << "test_for_no_cond: [1] bad\n"; return 3; }
    if (!asI32Const(instrs[2]))                  { std::cerr << "test_for_no_cond: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Return))  { std::cerr << "test_for_no_cond: [3] bad\n"; return 5; }
    auto br = asOneIdx(instrs[4], WasmVM::Opcode::Br);
    if (!br || br->index != 0)                   { std::cerr << "test_for_no_cond: [4] bad\n"; return 6; }
    if (!is(instrs[5], WasmVM::Opcode::End))     { std::cerr << "test_for_no_cond: [5] bad\n"; return 7; }
    if (!is(instrs[6], WasmVM::Opcode::End))     { std::cerr << "test_for_no_cond: [6] bad\n"; return 8; }
    return 0;
}

// for (;1;) return 0;
// →  [Block, Loop, I32_const{1}, I32_eqz, Br_if{1}, I32_const{0}, Return, Br{0}, End, End]
static int test_for_with_cond() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto fs = make_ast<ForStmt>();
    fs->kind = Stmt::Kind::For;
    fs->cond = makeI32(1);
    fs->body = makeReturnStmt(makeI32(0));

    codegen.emitStmt(fs);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 10) {
        std::cerr << "test_for_with_cond: expected 10 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Block))           { std::cerr << "test_for_with_cond: [0] bad\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::Loop))            { std::cerr << "test_for_with_cond: [1] bad\n"; return 3; }
    if (!asI32Const(instrs[2]))                          { std::cerr << "test_for_with_cond: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::I32_eqz))         { std::cerr << "test_for_with_cond: [3] bad\n"; return 5; }
    auto brif = asOneIdx(instrs[4], WasmVM::Opcode::Br_if);
    if (!brif || brif->index != 1)                       { std::cerr << "test_for_with_cond: [4] bad\n"; return 6; }
    if (!asI32Const(instrs[5]))                          { std::cerr << "test_for_with_cond: [5] bad\n"; return 7; }
    if (!is(instrs[6], WasmVM::Opcode::Return))          { std::cerr << "test_for_with_cond: [6] bad\n"; return 8; }
    auto br = asOneIdx(instrs[7], WasmVM::Opcode::Br);
    if (!br || br->index != 0)                           { std::cerr << "test_for_with_cond: [7] bad\n"; return 9; }
    if (!is(instrs[8], WasmVM::Opcode::End))             { std::cerr << "test_for_with_cond: [8] bad\n"; return 10; }
    if (!is(instrs[9], WasmVM::Opcode::End))             { std::cerr << "test_for_with_cond: [9] bad\n"; return 11; }
    return 0;
}

// for (;1; 5) return 0;
// →  [Block, Loop, I32_const{1}, I32_eqz, Br_if{1}, I32_const{0}, Return,
//     I32_const{5}, Drop, Br{0}, End, End]
static int test_for_with_step() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto fs = make_ast<ForStmt>();
    fs->kind = Stmt::Kind::For;
    fs->cond = makeI32(1);
    fs->step = makeI32(5);
    fs->body = makeReturnStmt(makeI32(0));

    codegen.emitStmt(fs);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 12) {
        std::cerr << "test_for_with_step: expected 12 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Block))           { std::cerr << "test_for_with_step: [0] bad\n"; return 2; }
    if (!is(instrs[1], WasmVM::Opcode::Loop))            { std::cerr << "test_for_with_step: [1] bad\n"; return 3; }
    if (!asI32Const(instrs[2]))                          { std::cerr << "test_for_with_step: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::I32_eqz))         { std::cerr << "test_for_with_step: [3] bad\n"; return 5; }
    auto brif = asOneIdx(instrs[4], WasmVM::Opcode::Br_if);
    if (!brif || brif->index != 1)                       { std::cerr << "test_for_with_step: [4] bad\n"; return 6; }
    if (!asI32Const(instrs[5]))                          { std::cerr << "test_for_with_step: [5] bad\n"; return 7; }
    if (!is(instrs[6], WasmVM::Opcode::Return))          { std::cerr << "test_for_with_step: [6] bad\n"; return 8; }
    auto stepC = asI32Const(instrs[7]);
    if (!stepC || stepC->value != 5)                     { std::cerr << "test_for_with_step: [7] bad\n"; return 9; }
    if (!is(instrs[8], WasmVM::Opcode::Drop))            { std::cerr << "test_for_with_step: [8] bad\n"; return 10; }
    auto br = asOneIdx(instrs[9], WasmVM::Opcode::Br);
    if (!br || br->index != 0)                           { std::cerr << "test_for_with_step: [9] bad\n"; return 11; }
    if (!is(instrs[10], WasmVM::Opcode::End))            { std::cerr << "test_for_with_step: [10] bad\n"; return 12; }
    if (!is(instrs[11], WasmVM::Opcode::End))            { std::cerr << "test_for_with_step: [11] bad\n"; return 13; }
    return 0;
}

// int x;  →  no instructions; one i32 local allocated
static int test_local_decl_no_init() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    codegen.emitBlockItem(wrapDecl(makeIntDecl("x")));
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (!instrs.empty()) {
        std::cerr << "test_local_decl_no_init: expected no instrs, got " << instrs.size() << "\n";
        return 1;
    }
    return 0;
}

// int x = 7;  →  [I32_const{7}, Local_set{0}]
static int test_local_decl_with_init() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    codegen.emitBlockItem(wrapDecl(makeIntDecl("x", makeI32(7))));
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 2) {
        std::cerr << "test_local_decl_with_init: expected 2 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 7) {
        std::cerr << "test_local_decl_with_init: [0] expected I32_const{7}\n";
        return 2;
    }
    auto ls = asOneIdx(instrs[1], WasmVM::Opcode::Local_set);
    if (!ls || ls->index != 0) {
        std::cerr << "test_local_decl_with_init: [1] expected Local_set{0}\n";
        return 3;
    }
    return 0;
}

// { int x = 7; return x; }
// →  [I32_const{7}, Local_set{0}, Local_get{0}, Return]
static int test_local_decl_then_read() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto xExpr = make_ast<IdentifierExpr>();
    xExpr->kind = Expr::Kind::Ident;
    xExpr->name = "x";

    auto cs = make_ast<CompoundStmt>();
    cs->kind = Stmt::Kind::Compound;
    cs->items.push_back(wrapDecl(makeIntDecl("x", makeI32(7))));
    cs->items.push_back(wrapStmt(makeReturnStmt(xExpr)));

    codegen.emitStmt(cs);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 4) {
        std::cerr << "test_local_decl_then_read: expected 4 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 7) { std::cerr << "test_local_decl_then_read: [0] bad\n"; return 2; }
    auto ls = asOneIdx(instrs[1], WasmVM::Opcode::Local_set);
    if (!ls || ls->index != 0) { std::cerr << "test_local_decl_then_read: [1] bad\n"; return 3; }
    auto lg = asOneIdx(instrs[2], WasmVM::Opcode::Local_get);
    if (!lg || lg->index != 0) { std::cerr << "test_local_decl_then_read: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Return)) { std::cerr << "test_local_decl_then_read: [3] bad\n"; return 5; }
    return 0;
}

// for (int i = 0;; ) return i;
// →  [I32_const{0}, Local_set{0}, Block, Loop, Local_get{0}, Return, Br{0}, End, End]
static int test_for_with_decl_init() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    auto iExpr = make_ast<IdentifierExpr>();
    iExpr->kind = Expr::Kind::Ident;
    iExpr->name = "i";

    auto fs = make_ast<ForStmt>();
    fs->kind = Stmt::Kind::For;
    fs->init = wrapDecl(makeIntDecl("i", makeI32(0)));
    fs->body = makeReturnStmt(iExpr);

    codegen.emitStmt(fs);

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 9) {
        std::cerr << "test_for_with_decl_init: expected 9 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 0) { std::cerr << "test_for_with_decl_init: [0] bad\n"; return 2; }
    auto ls = asOneIdx(instrs[1], WasmVM::Opcode::Local_set);
    if (!ls || ls->index != 0) { std::cerr << "test_for_with_decl_init: [1] bad\n"; return 3; }
    if (!is(instrs[2], WasmVM::Opcode::Block)) { std::cerr << "test_for_with_decl_init: [2] bad\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Loop))  { std::cerr << "test_for_with_decl_init: [3] bad\n"; return 5; }
    auto lg = asOneIdx(instrs[4], WasmVM::Opcode::Local_get);
    if (!lg || lg->index != 0) { std::cerr << "test_for_with_decl_init: [4] bad\n"; return 6; }
    if (!is(instrs[5], WasmVM::Opcode::Return)) { std::cerr << "test_for_with_decl_init: [5] bad\n"; return 7; }
    auto br = asOneIdx(instrs[6], WasmVM::Opcode::Br);
    if (!br || br->index != 0) { std::cerr << "test_for_with_decl_init: [6] bad\n"; return 8; }
    if (!is(instrs[7], WasmVM::Opcode::End)) { std::cerr << "test_for_with_decl_init: [7] bad\n"; return 9; }
    if (!is(instrs[8], WasmVM::Opcode::End)) { std::cerr << "test_for_with_decl_init: [8] bad\n"; return 10; }
    return 0;
}

// Acceptance criteria: int add(int a, int b) { return a+b; }
// →  [Local_get{0}, Local_get{1}, I32_add, Return]
static int test_acceptance_add_function() {
    TypeMap typeMap;
    SymbolTable symbolTable;
    FunctionCodegen codegen(typeMap, symbolTable);

    symbolTable.pushScope();
    ScalarLocal infoA; infoA.type = nullptr; infoA.isAddressTaken = false; infoA.localIndex = 0;
    ScalarLocal infoB; infoB.type = nullptr; infoB.isAddressTaken = false; infoB.localIndex = 1;
    symbolTable.define("a", infoA);
    symbolTable.define("b", infoB);

    auto aExpr = make_ast<IdentifierExpr>();
    aExpr->kind = Expr::Kind::Ident;
    aExpr->name = "a";

    auto bExpr = make_ast<IdentifierExpr>();
    bExpr->kind = Expr::Kind::Ident;
    bExpr->name = "b";

    auto addExpr = make_ast<BinaryExpr>();
    addExpr->kind = Expr::Kind::Binary;
    addExpr->op = "+";
    addExpr->lhs = aExpr;
    addExpr->rhs = bExpr;

    codegen.emitStmt(makeReturnStmt(addExpr));
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 4) {
        std::cerr << "test_acceptance_add_function: expected 4 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto lg0 = asOneIdx(instrs[0], WasmVM::Opcode::Local_get);
    if (!lg0 || lg0->index != 0) { std::cerr << "test_acceptance_add_function: [0] expected Local_get{0}\n"; return 2; }
    auto lg1 = asOneIdx(instrs[1], WasmVM::Opcode::Local_get);
    if (!lg1 || lg1->index != 1) { std::cerr << "test_acceptance_add_function: [1] expected Local_get{1}\n"; return 3; }
    if (!is(instrs[2], WasmVM::Opcode::I32_add)) { std::cerr << "test_acceptance_add_function: [2] expected I32_add\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Return))  { std::cerr << "test_acceptance_add_function: [3] expected Return\n"; return 5; }
    return 0;
}

// return add(1, 2);  where add is FuncSymbol at index 0
// →  [I32_const{1}, I32_const{2}, Call{0}, Return]
static int test_return_call() {
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

    codegen.emitStmt(makeReturnStmt(callExpr));
    symbolTable.popScope();

    const auto& instrs = codegen.getInstructions();
    if (instrs.size() != 4) {
        std::cerr << "test_return_call: expected 4 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c1 = asI32Const(instrs[0]);
    if (!c1 || c1->value != 1) { std::cerr << "test_return_call: [0] expected I32_const{1}\n"; return 2; }
    auto c2 = asI32Const(instrs[1]);
    if (!c2 || c2->value != 2) { std::cerr << "test_return_call: [1] expected I32_const{2}\n"; return 3; }
    auto call = asOneIdx(instrs[2], WasmVM::Opcode::Call);
    if (!call || call->index != 0) { std::cerr << "test_return_call: [2] expected Call{0}\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::Return)) { std::cerr << "test_return_call: [3] expected Return\n"; return 5; }
    return 0;
}

// ---------------------------------------------------------------------------
// Issue #10: MemoryLocal promotion and prologue/epilogue
// ---------------------------------------------------------------------------

// Simulate: int f() { int x; (&x); return 0; }
// x is address-taken → MemoryLocal, frameSize = 4 (sizeof int)
// Expected final instructions:
//   [0]  Global_get{0}          // prologue: save SP
//   [1]  Local_tee{0}           // prologue: fp = saved SP
//   [2]  I64_const{4}           // prologue: frameSize
//   [3]  I64_sub                // prologue: new SP = fp - frameSize
//   [4]  Global_set{0}          // prologue: update SP
//   [5]  Unreachable            // emitExpr(x) for MemoryLocal (not yet implemented)
//   [6]  Unreachable            // emitUnaryExpr('&') not yet implemented
//   [7]  Drop                   // ExprStmt
//   [8]  I32_const{0}           // return 0
//   [9]  Local_get{0}           // epilogue before return: get fp
//   [10] Global_set{0}          // epilogue before return: restore SP
//   [11] Return
//   [12] Local_get{0}           // epilogue at end (fallthrough)
//   [13] Global_set{0}
static int test_memory_local_prologue_epilogue() {
    TypeMap typeMap;
    SymbolTable symbolTable;

    auto funcDef = make_ast<FunctionDef>();

    // int x;
    auto xDeclItem = wrapDecl(makeIntDecl("x"));

    // (&x)
    auto xIdent = make_ast<IdentifierExpr>();
    xIdent->kind = Expr::Kind::Ident;
    xIdent->name = "x";

    auto addrX = make_ast<UnaryExpr>();
    addrX->kind = Expr::Kind::Unary;
    addrX->op = "&";
    addrX->rhs = xIdent;

    auto addrXStmtItem = wrapStmt(makeExprStmt(addrX));
    auto ret0Item = wrapStmt(makeReturnStmt(makeI32(0)));

    funcDef->body.push_back(xDeclItem);
    funcDef->body.push_back(addrXStmtItem);
    funcDef->body.push_back(ret0Item);

    auto tu = make_ast<TranslationUnit>();
    wvmcc::parser::Semantic semantic(tu);

    FunctionCodegen codegen(typeMap, symbolTable);
    symbolTable.pushScope();
    auto wasmFunc = codegen.generate(funcDef, semantic);
    symbolTable.popScope();

    const auto& instrs = wasmFunc.body;

    if (instrs.size() < 5) {
        std::cerr << "test_memory_local_prologue_epilogue: too few instrs (" << instrs.size() << ")\n";
        return 1;
    }
    if (!is(instrs[0], WasmVM::Opcode::Global_get)) {
        std::cerr << "test_memory_local_prologue_epilogue: [0] expected Global_get\n"; return 2;
    }
    auto ltee = asOneIdx(instrs[1], WasmVM::Opcode::Local_tee);
    if (!ltee) {
        std::cerr << "test_memory_local_prologue_epilogue: [1] expected Local_tee\n"; return 3;
    }
    auto fsize = asI64Const(instrs[2]);
    if (!fsize || fsize->value != 4) {
        std::cerr << "test_memory_local_prologue_epilogue: [2] expected I64_const{4}, got "
                  << (fsize ? std::to_string(fsize->value) : "?") << "\n"; return 4;
    }
    if (!is(instrs[3], WasmVM::Opcode::I64_sub)) {
        std::cerr << "test_memory_local_prologue_epilogue: [3] expected I64_sub\n"; return 5;
    }
    if (!is(instrs[4], WasmVM::Opcode::Global_set)) {
        std::cerr << "test_memory_local_prologue_epilogue: [4] expected Global_set\n"; return 6;
    }

    // (&x) now correctly emits: Local_get{fp}, I64_const{0}, I64_add, Drop  (4 instrs)
    // return 0: I32_const{0}, Local_get{fp}, Global_set, Return              (4 instrs)
    // epilogue at end: Local_get{fp}, Global_set                             (2 instrs)
    // Total: 5 prologue + 4 body + 4 return + 2 epilogue = 15
    if (instrs.size() != 15) {
        std::cerr << "test_memory_local_prologue_epilogue: expected 15 instrs, got " << instrs.size() << "\n";
        return 7;
    }

    // epilogue before Return: instrs[10]=Local_get{fp}, [11]=Global_set, [12]=Return
    auto fpGet = asOneIdx(instrs[10], WasmVM::Opcode::Local_get);
    if (!fpGet || fpGet->index != ltee->index) {
        std::cerr << "test_memory_local_prologue_epilogue: [10] expected Local_get{fp}\n"; return 8;
    }
    if (!is(instrs[11], WasmVM::Opcode::Global_set)) {
        std::cerr << "test_memory_local_prologue_epilogue: [11] expected Global_set\n"; return 9;
    }
    if (!is(instrs[12], WasmVM::Opcode::Return)) {
        std::cerr << "test_memory_local_prologue_epilogue: [12] expected Return\n"; return 10;
    }

    // epilogue at end: instrs[13]=Local_get{fp}, [14]=Global_set
    auto fpGet2 = asOneIdx(instrs[13], WasmVM::Opcode::Local_get);
    if (!fpGet2 || fpGet2->index != ltee->index) {
        std::cerr << "test_memory_local_prologue_epilogue: [13] expected Local_get{fp}\n"; return 11;
    }
    if (!is(instrs[14], WasmVM::Opcode::Global_set)) {
        std::cerr << "test_memory_local_prologue_epilogue: [14] expected Global_set\n"; return 12;
    }
    return 0;
}

// Without address-taken variables there should be no prologue/epilogue
// int f() { return 7; }  →  [I32_const{7}, Return]
static int test_no_prologue_without_address_taken() {
    TypeMap typeMap;
    SymbolTable symbolTable;

    auto funcDef = make_ast<FunctionDef>();
    funcDef->body.push_back(wrapStmt(makeReturnStmt(makeI32(7))));

    auto tu = make_ast<TranslationUnit>();
    wvmcc::parser::Semantic semantic(tu);

    FunctionCodegen codegen(typeMap, symbolTable);
    symbolTable.pushScope();
    auto wasmFunc = codegen.generate(funcDef, semantic);
    symbolTable.popScope();

    const auto& instrs = wasmFunc.body;
    if (instrs.size() != 2) {
        std::cerr << "test_no_prologue_without_address_taken: expected 2 instrs, got " << instrs.size() << "\n";
        return 1;
    }
    auto c = asI32Const(instrs[0]);
    if (!c || c->value != 7) {
        std::cerr << "test_no_prologue_without_address_taken: [0] expected I32_const{7}\n"; return 2;
    }
    if (!is(instrs[1], WasmVM::Opcode::Return)) {
        std::cerr << "test_no_prologue_without_address_taken: [1] expected Return\n"; return 3;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

#define RUN(fn) \
    do { \
        int r = fn(); \
        if (r != 0) { std::cerr << #fn " FAILED (code " << r << ")\n"; return r; } \
        std::cout << #fn " passed\n"; \
    } while (0)

// Helper: build int* declaration with initializer
static DeclarationPtr makeIntPtrDecl(const std::string& name, ExprPtr initExpr = nullptr) {
    auto decl = make_ast<Declaration>();
    decl->declarator = make_ast<Declarator>();
    decl->declarator->kind = Declarator::Kind::Identifier;
    decl->declarator->id.name = name;
    // inner pointer declarator
    auto inner = make_ast<Declarator>();
    inner->kind = Declarator::Kind::Pointer;
    decl->declarator->inner = inner;

    DeclarationSpecifiers::TypeSpecifier ts;
    ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
    ts.simple = {DeclarationSpecifiers::SimpleTypeSpecifier::Int};
    decl->specifiers.typeSpecifiers.push_back(ts);

    if (initExpr) {
        auto init = make_ast<Initializer>();
        init->kind = Initializer::Kind::Expr;
        init->expr = initExpr;
        decl->initializer = init;
    }
    return decl;
}

// Helper: make a unary expression
static ExprPtr makeUnary(const std::string& op, ExprPtr rhs) {
    auto e = make_ast<UnaryExpr>(); e->kind = Expr::Kind::Unary; e->op = op; e->rhs = rhs; return e;
}

// Helper: make a binary expression
static ExprPtr makeBinary(const std::string& op, ExprPtr lhs, ExprPtr rhs) {
    auto e = make_ast<BinaryExpr>(); e->kind = Expr::Kind::Binary; e->op = op; e->lhs = lhs; e->rhs = rhs; return e;
}

// Helper: make an identifier expression
static ExprPtr makeIdent(const std::string& name) {
    auto e = make_ast<IdentifierExpr>(); e->kind = Expr::Kind::Ident; e->name = name; return e;
}

// Helper: build a minimal void FunctionDef
static FunctionDefPtr makeVoidFuncDef(const std::string& name, std::vector<BlockItemPtr> body) {
    auto f = make_ast<FunctionDef>();
    DeclarationSpecifiers::TypeSpecifier voidTs;
    voidTs.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
    voidTs.simple = {DeclarationSpecifiers::SimpleTypeSpecifier::Void};
    f->specifiers.typeSpecifiers.push_back(voidTs);
    f->declarator = make_ast<Declarator>();
    f->declarator->id.name = name;
    f->body = std::move(body);
    return f;
}

// AC1: int x; int* p = &x; *p = 5;
// Verifies:
//   (a) shadow-stack prologue is emitted (x is address-taken)
//   (b) &x emits Local_get{fp}, I64_const{0}, I64_add (correct shadow-stack address)
//   (c) *p = 5 emits pointer load + store through mem[0]
static int test_ac1_shadow_stack_address_sequence() {
    // int x;
    auto xDecl = makeIntDecl("x");
    // int* p = &x;
    auto pDecl = makeIntPtrDecl("p", makeUnary("&", makeIdent("x")));
    // *p = 5;
    auto assignStmt = makeExprStmt(makeBinary("=", makeUnary("*", makeIdent("p")), makeI32(5)));

    auto funcDef = makeVoidFuncDef("test_ac1", {wrapDecl(xDecl), wrapDecl(pDecl), wrapStmt(assignStmt)});

    auto tu = make_ast<TranslationUnit>();
    Semantic semantic(tu);
    TypeMap typeMap; SymbolTable symtab;
    FunctionCodegen cg(typeMap, symtab);
    symtab.pushScope();
    auto wf = cg.generate(funcDef, semantic);
    symtab.popScope();

    const auto& instrs = wf.body;

    // Prologue (x is address-taken → shadow stack): Global_get, Local_tee{fp}, I64_const{4}, I64_sub, Global_set
    if (instrs.size() < 5) { std::cerr << "AC1: too few instrs (" << instrs.size() << ")\n"; return 1; }
    if (!is(instrs[0], WasmVM::Opcode::Global_get)) { std::cerr << "AC1: [0] expected Global_get (prologue)\n"; return 2; }
    auto ltee = asOneIdx(instrs[1], WasmVM::Opcode::Local_tee);
    if (!ltee) { std::cerr << "AC1: [1] expected Local_tee{fp} (prologue)\n"; return 3; }
    auto fsize = asI64Const(instrs[2]);
    if (!fsize || fsize->value != 4) { std::cerr << "AC1: [2] expected I64_const{4} (frame size=4 for int x)\n"; return 4; }
    if (!is(instrs[3], WasmVM::Opcode::I64_sub)) { std::cerr << "AC1: [3] expected I64_sub (prologue)\n"; return 5; }
    if (!is(instrs[4], WasmVM::Opcode::Global_set)) { std::cerr << "AC1: [4] expected Global_set (prologue)\n"; return 6; }

    // &x → shadow-stack address of x at offset 0: Local_get{fp}, I64_const{0}, I64_add
    // followed by Local_set{p}
    auto lgFp = asOneIdx(instrs[5], WasmVM::Opcode::Local_get);
    if (!lgFp || lgFp->index != ltee->index) { std::cerr << "AC1: [5] expected Local_get{fp} (&x part 1)\n"; return 7; }
    auto xOff = asI64Const(instrs[6]);
    if (!xOff || xOff->value != 0) { std::cerr << "AC1: [6] expected I64_const{0} (x at frame offset 0)\n"; return 8; }
    if (!is(instrs[7], WasmVM::Opcode::I64_add)) { std::cerr << "AC1: [7] expected I64_add (shadow-stack address of x)\n"; return 9; }
    if (!is(instrs[8], WasmVM::Opcode::Local_set)) { std::cerr << "AC1: [8] expected Local_set (p = &x)\n"; return 10; }

    // *p = 5: rhs eval, address via p, store through pointer
    auto five = asI32Const(instrs[9]);
    if (!five || five->value != 5) { std::cerr << "AC1: [9] expected I32_const{5}\n"; return 11; }
    if (!is(instrs[10], WasmVM::Opcode::Local_set)) { std::cerr << "AC1: [10] expected Local_set{temp}\n"; return 12; }
    if (!is(instrs[11], WasmVM::Opcode::Local_get)) { std::cerr << "AC1: [11] expected Local_get{p} (pointer value)\n"; return 13; }
    if (!is(instrs[12], WasmVM::Opcode::Local_get)) { std::cerr << "AC1: [12] expected Local_get{temp} (value 5)\n"; return 14; }
    if (!is(instrs[13], WasmVM::Opcode::I32_store)) { std::cerr << "AC1: [13] expected I32_store (write through *p)\n"; return 15; }

    return 0;
}

// AC2: struct S { int a; int b; }; S s; s.b
// Verifies: s.b emits the correct field offset (4 bytes past start of struct)
static int test_ac2_struct_field_offset() {
    // Build struct type: struct S { int a; int b; }
    auto su = std::make_shared<StructOrUnionSpecifier>();
    su->kind = StructOrUnionSpecifier::Kind::Struct;
    su->hasBody = true;
    su->name = "S";
    auto addField = [&](const std::string& fieldName) {
        StructMember m;
        DeclarationSpecifiers::TypeSpecifier ts;
        ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
        ts.simple = {DeclarationSpecifiers::SimpleTypeSpecifier::Int};
        m.specifiers.typeSpecifiers.push_back(ts);
        StructDeclarator sd;
        sd.declarator = make_ast<Declarator>();
        sd.declarator->kind = Declarator::Kind::Identifier;
        sd.declarator->id.name = fieldName;
        m.declarators.push_back(sd);
        su->members.push_back(m);
    };
    addField("a");
    addField("b");

    // struct S s;
    auto sDecl = make_ast<Declaration>();
    sDecl->declarator = make_ast<Declarator>();
    sDecl->declarator->kind = Declarator::Kind::Identifier;
    sDecl->declarator->id.name = "s";
    DeclarationSpecifiers::TypeSpecifier suTs;
    suTs.kind = DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion;
    suTs.su = su;
    sDecl->specifiers.typeSpecifiers.push_back(suTs);

    // s.b;
    auto sbExpr = make_ast<MemberExpr>();
    sbExpr->kind = Expr::Kind::Member;
    sbExpr->base = makeIdent("s");
    sbExpr->member = "b";
    sbExpr->isArrow = false;

    auto funcDef = makeVoidFuncDef("test_ac2", {wrapDecl(sDecl), wrapStmt(makeExprStmt(sbExpr))});

    auto tu = make_ast<TranslationUnit>();
    Semantic semantic(tu);
    TypeMap typeMap; SymbolTable symtab;
    FunctionCodegen cg(typeMap, symtab);
    symtab.pushScope();
    auto wf = cg.generate(funcDef, semantic);
    symtab.popScope();

    const auto& instrs = wf.body;

    // Prologue: 5 instrs (struct forces shadow stack even without explicit address-of)
    if (instrs.size() < 5) { std::cerr << "AC2: too few instrs\n"; return 1; }
    auto ltee = asOneIdx(instrs[1], WasmVM::Opcode::Local_tee);
    if (!ltee) { std::cerr << "AC2: [1] expected Local_tee{fp}\n"; return 2; }

    // s.b body (starts at [5]):
    //   [5] Local_get{fp}   — base of s (lvalue)
    //   [6] I64_const{0}    — frame offset of s
    //   [7] I64_add         — address of s
    //   [8] I64_const{4}    — field offset of b (after 4-byte 'a')
    //   [9] I64_add         — address of s.b
    //   [10] I32_load{1}    — load field from shadow-stack memory
    //   [11] Drop           — ExprStmt
    auto lgFp = asOneIdx(instrs[5], WasmVM::Opcode::Local_get);
    if (!lgFp || lgFp->index != ltee->index) { std::cerr << "AC2: [5] expected Local_get{fp}\n"; return 3; }
    auto sOff = asI64Const(instrs[6]);
    if (!sOff || sOff->value != 0) { std::cerr << "AC2: [6] expected I64_const{0} (s at frame offset 0)\n"; return 4; }
    if (!is(instrs[7], WasmVM::Opcode::I64_add)) { std::cerr << "AC2: [7] expected I64_add (address of s)\n"; return 5; }
    auto fieldOff = asI64Const(instrs[8]);
    if (!fieldOff || fieldOff->value != 4) {
        std::cerr << "AC2: [8] expected I64_const{4} (field 'b' at offset 4), got "
                  << (fieldOff ? std::to_string(fieldOff->value) : "?") << "\n"; return 6;
    }
    if (!is(instrs[9],  WasmVM::Opcode::I64_add))   { std::cerr << "AC2: [9] expected I64_add (address of s.b)\n"; return 7; }
    if (!is(instrs[10], WasmVM::Opcode::I32_load))   { std::cerr << "AC2: [10] expected I32_load (load s.b from mem[1])\n"; return 8; }
    if (!is(instrs[11], WasmVM::Opcode::Drop))        { std::cerr << "AC2: [11] expected Drop (ExprStmt)\n"; return 9; }

    return 0;
}

int main() {
    RUN(test_return_void);
    RUN(test_return_value);
    RUN(test_expr_stmt);
    RUN(test_empty_stmt);
    RUN(test_compound_stmt);
    RUN(test_if_no_else);
    RUN(test_if_with_else);
    RUN(test_while_stmt);
    RUN(test_for_no_cond);
    RUN(test_for_with_cond);
    RUN(test_for_with_step);
    RUN(test_local_decl_no_init);
    RUN(test_local_decl_with_init);
    RUN(test_local_decl_then_read);
    RUN(test_for_with_decl_init);
    RUN(test_acceptance_add_function);
    RUN(test_return_call);
    RUN(test_memory_local_prologue_epilogue);
    RUN(test_no_prologue_without_address_taken);
    RUN(test_ac1_shadow_stack_address_sequence);
    RUN(test_ac2_struct_field_offset);
    std::cout << "All statement emission tests passed!\n";
    return 0;
}
