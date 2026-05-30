// M2-L5: cross-module DCE removes unreachable defined functions.
#include "link/DeadCodeEliminator.hpp"
#include "link/LinkContext.hpp"

#include <cstdio>
#include <iostream>

using namespace wvmcc::link;
using namespace wvmcc::link::dce;

static int failures = 0;
#define EXPECT(cond, msg)                                                     \
    do {                                                                      \
        if (!(cond)) { std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg,        \
                                    __FILE__, __LINE__); ++failures; }        \
    } while (0)

int main() {
    // Module:
    //   types: [0]=() -> i32
    //   funcs: [0]=reachable_via_export,
    //          [1]=reachable_via_call (called by [0]),
    //          [2]=dead
    //   exports: "live" -> func 0
    //   start: func 0
    LinkContext ctx;
    auto& m = ctx.output;

    WasmVM::FuncType ft;
    ft.results.push_back(WasmVM::ValueType::i32);
    m.types.push_back(ft);

    auto makeFn = [&](WasmVM::index_t calls) {
        WasmVM::WasmFunc f;
        f.typeidx = 0;
        if (calls != (WasmVM::index_t)-1) {
            f.body.push_back(WasmVM::Instr::Call{calls});
        }
        f.body.push_back(WasmVM::Instr::I32_const{0});
        f.body.push_back(WasmVM::Instr::End{});
        return f;
    };
    m.funcs.push_back(makeFn(1));     // 0: reachable_via_export, calls 1
    m.funcs.push_back(makeFn((WasmVM::index_t)-1)); // 1: reachable_via_call
    m.funcs.push_back(makeFn((WasmVM::index_t)-1)); // 2: dead

    WasmVM::WasmExport ex;
    ex.name = "live";
    ex.desc = WasmVM::WasmExport::DescType::func;
    ex.index = 0;
    m.exports.push_back(ex);

    eliminate(ctx);

    EXPECT(m.funcs.size() == 2,
           "dead function removed; 2 of 3 funcs survive");

    // After DCE: func 0 stays at idx 0; old func 1 stays at idx 1; old
    // func 2 gone.
    EXPECT(m.exports.size() == 1 && m.exports[0].index == 0,
           "live export still at idx 0");

    if (m.funcs.size() == 2) {
        const auto& f0 = m.funcs[0];
        // f0 calls former func 1 → remap should keep target at 1.
        bool callsOne = false;
        for (const auto& instr : f0.body) {
            if (instr.opcode == WasmVM::Opcode::Call) {
                if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
                    callsOne = (oi->index == 1);
                }
            }
        }
        EXPECT(callsOne, "call target preserved after DCE remap");
    }

    // No-op case: all reachable, no removal.
    {
        LinkContext ctx2;
        auto& m2 = ctx2.output;
        m2.types.push_back(ft);
        m2.funcs.push_back(makeFn((WasmVM::index_t)-1));
        WasmVM::WasmExport e;
        e.name = "main";
        e.desc = WasmVM::WasmExport::DescType::func;
        e.index = 0;
        m2.exports.push_back(e);
        eliminate(ctx2);
        EXPECT(m2.funcs.size() == 1, "single reachable func not removed");
    }

    if (failures == 0) std::cout << "all dce tests passed\n";
    return failures == 0 ? 0 : 1;
}
