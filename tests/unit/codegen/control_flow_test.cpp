// Phase 3 control-flow tests: switch, do-while, break, continue, goto/label.
#include <cstdint>
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

static StmtPtr makeBreak()    { auto s = make_ast<BreakStmt>();    s->kind = Stmt::Kind::Break;    return s; }
static StmtPtr makeContinue() { auto s = make_ast<ContinueStmt>(); s->kind = Stmt::Kind::Continue; return s; }
static StmtPtr makeGoto(const std::string& l) {
    auto s = make_ast<GotoStmt>();
    s->kind = Stmt::Kind::Goto;
    s->label = l;
    return s;
}
static StmtPtr makeLabel(const std::string& name, StmtPtr inner) {
    auto s = make_ast<LabelStmt>();
    s->kind = Stmt::Kind::Label;
    s->name = name;
    s->stmt = inner;
    return s;
}
static StmtPtr makeCase(int32_t v, StmtPtr inner) {
    auto s = make_ast<CaseStmt>();
    s->kind = Stmt::Kind::Case;
    s->value = makeI32(v);
    s->stmt = inner;
    return s;
}
static StmtPtr makeDefault(StmtPtr inner) {
    auto s = make_ast<DefaultStmt>();
    s->kind = Stmt::Kind::Default;
    s->stmt = inner;
    return s;
}

static BlockItemPtr wrapStmt(StmtPtr stmt) {
    auto item = make_ast<BlockItem>();
    item->item = stmt;
    return item;
}

static StmtPtr makeCompound(std::vector<StmtPtr> stmts) {
    auto cs = make_ast<CompoundStmt>();
    cs->kind = Stmt::Kind::Compound;
    for (auto& s : stmts) cs->items.push_back(wrapStmt(s));
    return cs;
}

// ---------------------------------------------------------------------------
// Issue #18: do-while
// ---------------------------------------------------------------------------

// do { 1; } while (1);
//   →  Block, Loop, I32_const{1}, Drop, I32_const{1}, Br_if{0}, End, End
static int test_do_while_basic() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto dw = make_ast<DoWhileStmt>();
    dw->kind = Stmt::Kind::DoWhile;
    dw->body = makeExprStmt(makeI32(1));
    dw->cond = makeI32(1);

    cg.emitStmt(dw);
    const auto& I = cg.getInstructions();

    if (I.size() != 8) { std::cerr << "do_while_basic size " << I.size() << "\n"; return 1; }
    if (!is(I[0], WasmVM::Opcode::Block)) return 2;
    if (!is(I[1], WasmVM::Opcode::Loop)) return 3;
    if (!asI32Const(I[2])) return 4;
    if (!is(I[3], WasmVM::Opcode::Drop)) return 5;
    if (!asI32Const(I[4])) return 6;
    auto brif = asOneIdx(I[5], WasmVM::Opcode::Br_if);
    if (!brif || brif->index != 0) return 7;
    if (!is(I[6], WasmVM::Opcode::End)) return 8;
    if (!is(I[7], WasmVM::Opcode::End)) return 9;
    return 0;
}

// ---------------------------------------------------------------------------
// Issue #19: break, continue, goto, label
// ---------------------------------------------------------------------------

// while (1) { break; }
// expected break Br index = 1 (out of Loop, out of Block)
static int test_break_in_while() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto ws = make_ast<WhileStmt>();
    ws->kind = Stmt::Kind::While;
    ws->cond = makeI32(1);
    ws->body = makeBreak();

    cg.emitStmt(ws);
    const auto& I = cg.getInstructions();

    // Block, Loop, I32_const{1}, I32_eqz, Br_if{1}, Br{1} (break), Br{0} (loop back), End, End
    if (I.size() != 9) { std::cerr << "break_in_while size " << I.size() << "\n"; return 1; }
    auto br = asOneIdx(I[5], WasmVM::Opcode::Br);
    if (!br || br->index != 1) { std::cerr << "break_in_while: [5] expected Br{1}\n"; return 2; }
    return 0;
}

// while (1) { continue; }
// continue Br index = 0 (back to inner Loop)
static int test_continue_in_while() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto ws = make_ast<WhileStmt>();
    ws->kind = Stmt::Kind::While;
    ws->cond = makeI32(1);
    ws->body = makeContinue();

    cg.emitStmt(ws);
    const auto& I = cg.getInstructions();

    // Block, Loop, cond, I32_eqz, Br_if{1}, Br{0} (continue), Br{0} (loop back), End, End
    if (I.size() != 9) { std::cerr << "continue_in_while size " << I.size() << "\n"; return 1; }
    auto br = asOneIdx(I[5], WasmVM::Opcode::Br);
    if (!br || br->index != 0) { std::cerr << "continue_in_while: [5] expected Br{0}\n"; return 2; }
    return 0;
}

// while (1) { while (1) { break; } break; }
// inner break: Br{1}; outer break: Br{1}.  The inner break does NOT see
// the outer loop, so adding an outer loop must increment its depth correctly.
static int test_nested_break_depths() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    // Inner: while (1) { break; }
    auto innerWhile = make_ast<WhileStmt>();
    innerWhile->kind = Stmt::Kind::While;
    innerWhile->cond = makeI32(1);
    innerWhile->body = makeBreak();

    auto outerBody = makeCompound({std::static_pointer_cast<Stmt>(innerWhile), makeBreak()});

    auto outer = make_ast<WhileStmt>();
    outer->kind = Stmt::Kind::While;
    outer->cond = makeI32(1);
    outer->body = outerBody;

    cg.emitStmt(outer);
    const auto& I = cg.getInstructions();

    // Find the two Br{N} instructions that are the breaks (not the loop-back Br{0}).
    // Outer layout:
    //   [0] Block (outer break)
    //   [1] Loop  (outer cont)
    //   [2..] cond, eqz, br_if{1}
    //   [5] Block (inner break)
    //   [6] Loop  (inner cont)
    //   [7..] cond, eqz, br_if{1}
    //   [10] Br{1}  (inner break)
    //   [11] Br{0}  (inner loop-back)
    //   [12] End
    //   [13] End
    //   [14] Br{1}  (outer break)
    //   [15] Br{0}  (outer loop-back)
    //   [16] End
    //   [17] End
    auto innerBreak = asOneIdx(I[10], WasmVM::Opcode::Br);
    if (!innerBreak || innerBreak->index != 1) {
        std::cerr << "nested_break: [10] expected inner Br{1}\n"; return 1;
    }
    auto outerBreak = asOneIdx(I[14], WasmVM::Opcode::Br);
    if (!outerBreak || outerBreak->index != 1) {
        std::cerr << "nested_break: [14] expected outer Br{1}\n"; return 2;
    }
    return 0;
}

// while (1) { while (1) { continue; } }
// Inner continue should target inner loop with Br{0}.  Adding an extra outer
// loop must NOT change the inner continue depth.
static int test_nested_continue_depth() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto innerWhile = make_ast<WhileStmt>();
    innerWhile->kind = Stmt::Kind::While;
    innerWhile->cond = makeI32(1);
    innerWhile->body = makeContinue();

    auto outer = make_ast<WhileStmt>();
    outer->kind = Stmt::Kind::While;
    outer->cond = makeI32(1);
    outer->body = std::static_pointer_cast<Stmt>(innerWhile);

    cg.emitStmt(outer);
    const auto& I = cg.getInstructions();

    // [0] Block, [1] Loop, [2-4] outer cond, [5] Block, [6] Loop, [7-9] inner cond,
    // [10] Br{0} (continue), [11] Br{0} (inner loop-back), ...
    auto contBr = asOneIdx(I[10], WasmVM::Opcode::Br);
    if (!contBr || contBr->index != 0) {
        std::cerr << "nested_continue: [10] expected Br{0}\n"; return 1;
    }
    return 0;
}

// goto L; 1; L: 2;  →  Block, Br{0}, I32_const{1}, Drop, End, I32_const{2}, Drop
static int test_forward_goto_basic() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto cs = make_ast<CompoundStmt>();
    cs->kind = Stmt::Kind::Compound;
    cs->items.push_back(wrapStmt(makeGoto("L")));
    cs->items.push_back(wrapStmt(makeExprStmt(makeI32(1))));
    cs->items.push_back(wrapStmt(makeLabel("L", makeExprStmt(makeI32(2)))));

    cg.emitStmt(cs);
    const auto& I = cg.getInstructions();

    if (I.size() != 7) { std::cerr << "fwd_goto size " << I.size() << "\n"; return 1; }
    if (!is(I[0], WasmVM::Opcode::Block)) return 2;
    auto br = asOneIdx(I[1], WasmVM::Opcode::Br);
    if (!br || br->index != 0) return 3;
    if (!asI32Const(I[2])) return 4;
    if (!is(I[3], WasmVM::Opcode::Drop)) return 5;
    if (!is(I[4], WasmVM::Opcode::End)) return 6;
    if (!asI32Const(I[5])) return 7;
    if (!is(I[6], WasmVM::Opcode::Drop)) return 8;
    if (cg.getDiagnostics().size() != 0) { std::cerr << "unexpected diagnostics\n"; return 9; }
    return 0;
}

// L: 1; goto L;  →  backward goto: Unreachable + diagnostic
static int test_backward_goto_unreachable() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto cs = make_ast<CompoundStmt>();
    cs->kind = Stmt::Kind::Compound;
    cs->items.push_back(wrapStmt(makeLabel("L", makeExprStmt(makeI32(1)))));
    cs->items.push_back(wrapStmt(makeGoto("L")));

    cg.emitStmt(cs);
    const auto& I = cg.getInstructions();

    // I32_const{1}, Drop, Unreachable
    if (I.size() != 3) { std::cerr << "bwd_goto size " << I.size() << "\n"; return 1; }
    if (!is(I[2], WasmVM::Opcode::Unreachable)) return 2;
    if (cg.getDiagnostics().size() != 1) { std::cerr << "expected 1 diagnostic\n"; return 3; }
    return 0;
}

// ---------------------------------------------------------------------------
// Issue #17: switch lowering
// ---------------------------------------------------------------------------

// Dense switch: switch(1) { case 0: 100; break; case 1: 200; break; case 2: 300; break; }
// Expects a Br_table.
static int test_switch_dense_brtable() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto ss = make_ast<SwitchStmt>();
    ss->kind = Stmt::Kind::Switch;
    ss->cond = makeI32(1);
    ss->body = makeCompound({
        makeCase(0, makeExprStmt(makeI32(100))),
        makeBreak(),
        makeCase(1, makeExprStmt(makeI32(200))),
        makeBreak(),
        makeCase(2, makeExprStmt(makeI32(300))),
        makeBreak(),
    });

    cg.emitStmt(ss);
    const auto& I = cg.getInstructions();

    // Find the Br_table instruction
    bool seenBrTable = false;
    for (const auto& ins : I) {
        if (ins.opcode == WasmVM::Opcode::Br_table) { seenBrTable = true; break; }
    }
    if (!seenBrTable) { std::cerr << "switch_dense: expected a Br_table\n"; return 1; }

    // Outer Block + 3 case Blocks emitted
    int blockCount = 0;
    for (const auto& ins : I) if (ins.opcode == WasmVM::Opcode::Block) ++blockCount;
    if (blockCount != 4) { std::cerr << "switch_dense: expected 4 Blocks, got " << blockCount << "\n"; return 2; }
    return 0;
}

// Sparse switch: switch(1) { case 0: 100; break; case 100: 200; break; }
// (range = 100, count = 2 → 100 > 4*2 → sparse)
static int test_switch_sparse_ifchain() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto ss = make_ast<SwitchStmt>();
    ss->kind = Stmt::Kind::Switch;
    ss->cond = makeI32(1);
    ss->body = makeCompound({
        makeCase(0,   makeExprStmt(makeI32(100))),
        makeBreak(),
        makeCase(100, makeExprStmt(makeI32(200))),
        makeBreak(),
    });

    cg.emitStmt(ss);
    const auto& I = cg.getInstructions();

    // Sparse path uses Local_set / Local_get and chained I32_eq+Br_if; no Br_table
    bool seenBrTable = false;
    for (const auto& ins : I) if (ins.opcode == WasmVM::Opcode::Br_table) seenBrTable = true;
    if (seenBrTable) { std::cerr << "switch_sparse: unexpected Br_table\n"; return 1; }

    int eqCount = 0;
    for (const auto& ins : I) if (ins.opcode == WasmVM::Opcode::I32_eq) ++eqCount;
    if (eqCount < 2) { std::cerr << "switch_sparse: expected >=2 I32_eq, got " << eqCount << "\n"; return 2; }
    return 0;
}

// Switch with break exits the switch: switch(1) { case 0: break; }
static int test_switch_break_exits() {
    TypeMap tm; SymbolTable st;
    FunctionCodegen cg(tm, st);

    auto ss = make_ast<SwitchStmt>();
    ss->kind = Stmt::Kind::Switch;
    ss->cond = makeI32(1);
    ss->body = makeCompound({makeCase(0, makeBreak())});

    cg.emitStmt(ss);
    const auto& I = cg.getInstructions();

    // The break should produce a Br with depth that reaches the outer break block.
    // Layout: Block (break)+Block (case0)+<dispatch>+End+Br{depth}+End
    // From inside one open Block (after closing case0's block), depth to break = 0.
    bool foundBreakBr = false;
    for (size_t i = 0; i < I.size(); ++i) {
        if (I[i].opcode == WasmVM::Opcode::Br) {
            // Any Br is a candidate; just confirm at least one exists
            foundBreakBr = true;
            break;
        }
    }
    if (!foundBreakBr) { std::cerr << "switch_break_exits: expected a Br for break\n"; return 1; }
    return 0;
}

#define RUN(fn) \
    do { \
        int r = fn(); \
        if (r != 0) { std::cerr << #fn " FAILED (code " << r << ")\n"; return r; } \
        std::cout << #fn " passed\n"; \
    } while (0)

int main() {
    RUN(test_do_while_basic);
    RUN(test_break_in_while);
    RUN(test_continue_in_while);
    RUN(test_nested_break_depths);
    RUN(test_nested_continue_depth);
    RUN(test_forward_goto_basic);
    RUN(test_backward_goto_unreachable);
    RUN(test_switch_dense_brtable);
    RUN(test_switch_sparse_ifchain);
    RUN(test_switch_break_exits);
    std::cout << "All control-flow tests passed!\n";
    return 0;
}
