// M2-L3: resolve cross-module function imports against linked-TU exports.
// Host-runtime imports (sys_proc.*, sys_fs.*) are left alone.
#include "link/LinkContext.hpp"
#include "link/SymbolResolver.hpp"

#include <cstdio>
#include <iostream>
#include <string>

using namespace wvmcc::link;
using namespace wvmcc::link::resolve;

static int failures = 0;
#define EXPECT(cond, msg)                                                     \
    do {                                                                      \
        if (!(cond)) { std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg,        \
                                    __FILE__, __LINE__); ++failures; }        \
    } while (0)

int main() {
    // Construct a tiny merged module by hand:
    //   imports:
    //     [0] env.puts        (func, type 0)
    //     [1] sys_proc.exit   (func, type 1)
    //   funcs:
    //     [0] puts            (defined, type 0) — exported as "puts"
    //     [1] main            (defined, type 2) — calls Call 0 (env.puts)
    //                                         and Call 1 (sys_proc.exit)
    LinkContext ctx;
    auto& m = ctx.output;

    WasmVM::FuncType ft0;
    ft0.params.push_back(WasmVM::ValueType::i64);
    ft0.results.push_back(WasmVM::ValueType::i32);
    m.types.push_back(ft0);

    WasmVM::FuncType ft1;
    ft1.params.push_back(WasmVM::ValueType::i32);
    m.types.push_back(ft1);

    WasmVM::FuncType ft2;
    ft2.results.push_back(WasmVM::ValueType::i32);
    m.types.push_back(ft2);

    {
        WasmVM::WasmImport imp;
        imp.module = "env"; imp.name = "puts"; imp.desc = (WasmVM::index_t)0;
        m.imports.push_back(imp);
    }
    {
        WasmVM::WasmImport imp;
        imp.module = "sys_proc"; imp.name = "exit"; imp.desc = (WasmVM::index_t)1;
        m.imports.push_back(imp);
    }

    // Defined puts (func idx 2 after the two imports).
    {
        WasmVM::WasmFunc f;
        f.typeidx = 0;
        f.body.push_back(WasmVM::Instr::I32_const{0});
        f.body.push_back(WasmVM::Instr::End{});
        m.funcs.push_back(f);
    }
    // Defined main (func idx 3).
    {
        WasmVM::WasmFunc f;
        f.typeidx = 2;
        f.body.push_back(WasmVM::Instr::I64_const{0});
        f.body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)0});  // env.puts
        f.body.push_back(WasmVM::Instr::Drop{});
        f.body.push_back(WasmVM::Instr::I32_const{0});
        f.body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)1});  // sys_proc.exit
        f.body.push_back(WasmVM::Instr::End{});
        m.funcs.push_back(f);
    }

    // Exports
    {
        WasmVM::WasmExport ex;
        ex.name = "puts"; ex.desc = WasmVM::WasmExport::DescType::func;
        ex.index = 2; // points at the defined puts
        m.exports.push_back(ex);
    }

    resolveImports(ctx);

    // After resolve:
    // - env.puts dropped → imports has 1 entry (sys_proc.exit).
    // - puts is now at funcidx 1 (was 2; -1 because 1 import dropped).
    // - main is at funcidx 2.
    // - Call 0 in main rewritten to Call 1 (resolved puts).
    // - Call 1 (sys_proc.exit) shifted down by 1 → Call 0.

    EXPECT(m.imports.size() == 1, "imports drop env.puts; sys_proc.exit remains");
    EXPECT(m.imports[0].module == "sys_proc" && m.imports[0].name == "exit",
           "the remaining import is sys_proc.exit");

    const auto& mainFn = m.funcs[1];
    EXPECT(mainFn.body[1].opcode == WasmVM::Opcode::Call, "main[1] is a Call");
    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&mainFn.body[1].imm)) {
        EXPECT(oi->index == 1,
               "env.puts call now points at the local puts (new funcidx 1)");
    }
    EXPECT(mainFn.body[4].opcode == WasmVM::Opcode::Call, "main[4] is a Call");
    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&mainFn.body[4].imm)) {
        EXPECT(oi->index == 0,
               "sys_proc.exit call shifted down to funcidx 0");
    }

    // puts export's funcidx also shifted down by 1.
    bool putsExportOk = false;
    for (const auto& ex : m.exports) {
        if (ex.name == "puts") {
            putsExportOk = (ex.index == 1);
        }
    }
    EXPECT(putsExportOk, "puts export points at the new local index 1");

    if (failures == 0) std::cout << "all symbol-resolver tests passed\n";
    return failures == 0 ? 0 : 1;
}
