// Phase 4 unit tests — _Bool normalization at the codegen level. The Phase 4
// features that depend on a full ModuleCodegen (function tables / static
// locals / designated initializers for declarations) are covered by the
// end-to-end conformance suite under tests/standard/ because they need a real
// translation unit.
#include <cstdint>
#include <iostream>
#include "../../../src/codegen/FunctionCodegen.hpp"
#include "../../../src/codegen/TypeMap.hpp"
#include "../../../src/codegen/SymbolTable.hpp"
#include "../../../src/parser/AST.hpp"
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

static DeclarationPtr makeBoolDecl(const std::string& name, ExprPtr init) {
    auto d = make_ast<Declaration>();
    DeclarationSpecifiers::TypeSpecifier ts;
    ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
    ts.simple.push_back(DeclarationSpecifiers::SimpleTypeSpecifier::Bool);
    d->specifiers.typeSpecifiers.push_back(ts);
    d->declarator = make_ast<Declarator>();
    d->declarator->kind = Declarator::Kind::Identifier;
    d->declarator->id.name = name;
    auto i = make_ast<Initializer>();
    i->kind = Initializer::Kind::Expr;
    i->expr = init;
    d->initializer = i;
    return d;
}

// _Bool b = 5;
//   → I32_const{5}, I32_const{0}, I32_ne, Local_set{idx}
static int test_bool_init_normalizes() {
    TypeMap tm; SymbolTable st;
    st.pushScope();
    FunctionCodegen cg(tm, st);

    auto d = makeBoolDecl("b", makeI32(5));
    auto bi = make_ast<BlockItem>();
    bi->item = d;
    cg.emitBlockItem(bi);

    const auto& I = cg.getInstructions();
    if (I.size() != 4) {
        std::cerr << "bool_init: size " << I.size() << "\n"; return 1;
    }
    auto k0 = asI32Const(I[0]); if (!k0 || k0->value != 5)  return 2;
    auto k1 = asI32Const(I[1]); if (!k1 || k1->value != 0)  return 3;
    if (!is(I[2], WasmVM::Opcode::I32_ne)) return 4;
    if (!is(I[3], WasmVM::Opcode::Local_set)) return 5;
    return 0;
}

#define RUN(fn) \
    do { \
        int r = fn(); \
        if (r != 0) { std::cerr << #fn " FAILED (code " << r << ")\n"; return r; } \
        std::cout << #fn " passed\n"; \
    } while (0)

int main() {
    RUN(test_bool_init_normalizes);
    std::cout << "All Phase 4 unit tests passed!\n";
    return 0;
}
