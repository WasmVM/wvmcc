// --import-module extends the host-import allow-list.
//
// The linker treats an import from a host module (satisfied at instantiation)
// differently from an undefined reference (a symbol that must resolve to a
// local definition or an archive). That set was hardcoded to wasmvm's own
// sysenv (`sys_proc`, `sys_fs`); an embedder with its own host modules — here a
// stand-in `myhost` — had no way to declare them, so a linkable program importing
// from one failed on an undefined reference. `LinkOptions::import_modules`
// (from the flag) is the fix.
//
// Tested at the `link()` API, over a module shaped by hand, so it exercises the
// resolver and diagnostics directly rather than the CLI's comma-splitting
// (which the driver does before this point).
#include "link/Linker.hpp"

#include <cstdio>
#include <iostream>
#include <string>

static int failures = 0;
#define EXPECT(cond, msg)                                                     \
    do {                                                                      \
        if (!(cond)) { std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg,        \
                                    __FILE__, __LINE__); ++failures; }        \
    } while (0)

namespace {

// A module with one function import `myhost.log : () -> ()`. When `call_it` is
// true a defined, exported `run` calls it, so the import is *referenced*;
// otherwise the import is present but unused.
WasmVM::WasmModule moduleImporting(bool call_it) {
    WasmVM::WasmModule m;
    WasmVM::FuncType voidToVoid; // () -> ()
    m.types.push_back(voidToVoid);

    WasmVM::WasmImport imp;
    imp.module = "myhost";
    imp.name = "log";
    imp.desc = WasmVM::index_t{0}; // typeidx 0; function import -> funcidx 0
    m.imports.push_back(imp);

    WasmVM::WasmFunc fn; // becomes funcidx 1 (after the one import)
    fn.typeidx = 0;
    if (call_it) {
        fn.body.push_back(WasmVM::Instr::Call{WasmVM::index_t{0}}); // call myhost.log
    }
    fn.body.push_back(WasmVM::Instr::End{});
    m.funcs.push_back(fn);

    WasmVM::WasmExport ex;
    ex.name = "run";
    ex.desc = WasmVM::WasmExport::DescType::func;
    ex.index = 1;
    m.exports.push_back(ex);
    return m;
}

wvmcc::link::LinkResult linkWith(const WasmVM::WasmModule& m,
                                 std::vector<std::string> importModules) {
    using namespace wvmcc::link;
    LinkInput::InMemoryModule mm{m, "test.c"};
    LinkInput in;
    in.source = std::move(mm);
    std::vector<LinkInput> inputs;
    inputs.push_back(std::move(in));
    LinkOptions opts;
    opts.no_stdlib = true;
    opts.import_modules = std::move(importModules);
    return link(std::move(inputs), opts);
}

bool hasImport(const WasmVM::WasmModule& m, const char* module, const char* name) {
    for (const auto& imp : m.imports) {
        if (imp.module == module && imp.name == name) return true;
    }
    return false;
}

std::size_t importsFromModule(const WasmVM::WasmModule& m, const char* module) {
    std::size_t n = 0;
    for (const auto& imp : m.imports) if (imp.module == module) ++n;
    return n;
}

} // namespace

int main() {
    // ---- Default unchanged: an unknown host module is still undefined -------
    {
        auto r = linkWith(moduleImporting(/*call_it=*/true), {});
        EXPECT(!r.ok, "referenced import from an undeclared module is an error");
    }

    // ---- Declared: the import is preserved, the link succeeds ---------------
    {
        auto r = linkWith(moduleImporting(/*call_it=*/true), {"myhost"});
        EXPECT(r.ok, "--import-module=myhost makes the reference link");
        EXPECT(hasImport(r.module, "myhost", "log"), "myhost.log preserved as an import");
        EXPECT(importsFromModule(r.module, "sys_proc") == 0,
               "no sys_proc pulled in — it links without the process ABI");
    }

    // ---- A declared module's import is kept even when unreferenced ----------
    // A program may import a whole module and use one function; the rest must
    // survive, exactly as sys_proc's do.
    {
        auto r = linkWith(moduleImporting(/*call_it=*/false), {"myhost"});
        EXPECT(r.ok, "unused declared import links");
        EXPECT(hasImport(r.module, "myhost", "log"),
               "an unreferenced import from a declared module is retained");
    }

    // ---- Without the flag, an *unreferenced* unknown import is pruned, ------
    // not an error (C 6.9p5: a declared-but-unused extern imposes nothing).
    {
        auto r = linkWith(moduleImporting(/*call_it=*/false), {});
        EXPECT(r.ok, "unused undeclared import is dropped, not an error");
        EXPECT(!hasImport(r.module, "myhost", "log"), "the unused import was pruned");
    }

    // ---- Only the named module is admitted ---------------------------------
    {
        auto r = linkWith(moduleImporting(/*call_it=*/true), {"other"});
        EXPECT(!r.ok, "declaring a different module does not admit myhost");
    }

    if (failures == 0) std::cout << "all import-module tests passed\n";
    return failures == 0 ? 0 : 1;
}
