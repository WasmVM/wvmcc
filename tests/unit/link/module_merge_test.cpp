// M2-L2: merge multiple WasmModules into one, deduping types and imports
// and remapping cross-references.
#include "link/Linker.hpp"
#include "link/LinkContext.hpp"
#include "link/ModuleMerge.hpp"

#include <cstdio>
#include <iostream>
#include <string>

using namespace wvmcc::link;
using namespace wvmcc::link::merge;

static int failures = 0;
#define EXPECT(cond, msg)                                                     \
    do {                                                                      \
        if (!(cond)) { std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg,        \
                                    __FILE__, __LINE__); ++failures; }        \
    } while (0)

static WasmVM::WasmModule make_minimal_linkable(int answer,
                                                const std::string& exportName) {
    WasmVM::WasmModule m;

    // env.__linear_memory + env.__stack_memory imports (M2-D layout).
    auto memTy = []() {
        WasmVM::MemType t;
        t.min = 1;
        t.is64 = true;
        return t;
    }();
    for (const char* name : {"__linear_memory", "__stack_memory"}) {
        WasmVM::WasmImport imp;
        imp.module = "env";
        imp.name = name;
        imp.desc = memTy;
        m.imports.push_back(imp);
    }
    // env.__stack_pointer (mut i64).
    {
        WasmVM::WasmImport imp;
        imp.module = "env";
        imp.name = "__stack_pointer";
        imp.desc = WasmVM::GlobalType{WasmVM::GlobalType::variable, WasmVM::ValueType::i64};
        m.imports.push_back(imp);
    }
    // env.__heap_base (const i64).
    {
        WasmVM::WasmImport imp;
        imp.module = "env";
        imp.name = "__heap_base";
        imp.desc = WasmVM::GlobalType{WasmVM::GlobalType::constant, WasmVM::ValueType::i64};
        m.imports.push_back(imp);
    }

    // Type 0: () -> i32
    WasmVM::FuncType ft;
    ft.results.push_back(WasmVM::ValueType::i32);
    m.types.push_back(ft);

    // Func: returns `answer`.
    WasmVM::WasmFunc fn;
    fn.typeidx = 0;
    fn.body.push_back(WasmVM::Instr::I32_const{answer});
    fn.body.push_back(WasmVM::Instr::End{});
    m.funcs.push_back(fn);

    // Export the function.
    WasmVM::WasmExport ex;
    ex.name = exportName;
    ex.desc = WasmVM::WasmExport::DescType::func;
    ex.index = 0; // local func 0 (no func imports in this minimal module)
    m.exports.push_back(ex);

    return m;
}

int main() {
    // Two minimal linkable modules with different exports merge cleanly.
    auto m1 = make_minimal_linkable(11, "foo");
    auto m2 = make_minimal_linkable(22, "bar");

    LinkContext ctx;
    mergeOne(ctx, m1, "tu1.c");
    EXPECT(!ctx.hasErrors(), "merge tu1: no errors");
    mergeOne(ctx, m2, "tu2.c");
    EXPECT(!ctx.hasErrors(), "merge tu2: no errors");

    const auto& out = ctx.output;

    // Imports: 2 mems + 2 globals (deduped across the two TUs).
    int memImports = 0, globImports = 0;
    for (const auto& imp : out.imports) {
        if (std::holds_alternative<WasmVM::MemType>(imp.desc))    ++memImports;
        if (std::holds_alternative<WasmVM::GlobalType>(imp.desc)) ++globImports;
    }
    EXPECT(memImports == 2, "memory imports deduped to 2");
    EXPECT(globImports == 2, "global imports deduped to 2");

    // Types: deduped (both modules use the same () -> i32 signature).
    EXPECT(out.types.size() == 1, "function type deduplicated to 1");

    // Function defs: both appended.
    EXPECT(out.funcs.size() == 2, "two function defs in output");

    // Exports: both present, distinct names.
    EXPECT(out.exports.size() == 2, "two exports preserved");

    // The second export's func index should point at output func index 1
    // (= 0 func-imports + def slot 1).
    bool barFound = false, fooFound = false;
    for (const auto& ex : out.exports) {
        if (ex.name == "foo") { fooFound = true; EXPECT(ex.index == 0, "foo → func 0"); }
        if (ex.name == "bar") { barFound = true; EXPECT(ex.index == 1, "bar → func 1 (after remap)"); }
    }
    EXPECT(fooFound && barFound, "both export names present");

    // Duplicate export name → error.
    {
        LinkContext ctx2;
        auto a = make_minimal_linkable(1, "main");
        auto b = make_minimal_linkable(2, "main");
        mergeOne(ctx2, a, "a.c");
        mergeOne(ctx2, b, "b.c");
        EXPECT(ctx2.hasErrors(), "duplicate export name 'main' across TUs → error");
    }

    // Cross-TU call: m3 imports a func, m4 defines an unrelated func — the
    // import index in m3 should be deduped if a later TU re-imports the
    // same (module,name). M2-L3 will actually *resolve* the import; here we
    // just check the dedup mechanic.
    {
        LinkContext ctx3;
        auto a = make_minimal_linkable(1, "expA");
        auto b = make_minimal_linkable(2, "expB");
        // Add a `libc.puts` import to both.
        WasmVM::FuncType putsTy;
        putsTy.params.push_back(WasmVM::ValueType::i64);
        putsTy.results.push_back(WasmVM::ValueType::i32);
        auto inject_puts = [&](WasmVM::WasmModule& m) {
            // Intern the type.
            m.types.insert(m.types.begin(), putsTy);
            // Bump existing func.typeidx by 1.
            for (auto& f : m.funcs) ++f.typeidx;
            WasmVM::WasmImport imp;
            imp.module = "libc";
            imp.name = "puts";
            imp.desc = (WasmVM::index_t)0; // type 0 after the insert
            m.imports.insert(m.imports.begin(), imp);
            // The export at index 0 was local func 0 (no func imports); now
            // there's 1 func import, so the local func is at func index 1.
            for (auto& ex : m.exports) {
                if (ex.desc == WasmVM::WasmExport::DescType::func) ex.index = 1;
            }
        };
        inject_puts(a);
        inject_puts(b);

        mergeOne(ctx3, a, "a.c");
        mergeOne(ctx3, b, "b.c");
        EXPECT(!ctx3.hasErrors(), "merge with deduped libc.puts: no errors");

        // Output should have exactly ONE libc.puts func import.
        int putsCount = 0;
        for (const auto& imp : ctx3.output.imports) {
            if (imp.module == "libc" && imp.name == "puts") ++putsCount;
        }
        EXPECT(putsCount == 1, "libc.puts func import deduplicated to 1");
    }

    if (failures == 0) std::cout << "all module-merge tests passed\n";
    return failures == 0 ? 0 : 1;
}
