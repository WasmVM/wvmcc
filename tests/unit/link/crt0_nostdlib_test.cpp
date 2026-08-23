// `-nostdlib` materialises runtime state but not the process ABI.
//
// The flag used to return from `crt0::synthesize` before it had done anything,
// so a linkable module kept its `env.__*` mem/global imports and could not be
// instantiated by any host: those names are this compiler's internal
// declaration mechanism (docs/codegen.md), consumed by the link step, and no
// embedder provides them. Nor should one -- `__stack_pointer` is a mutable
// global, and sharing it would give two unrelated modules one stack pointer.
//
// Built from a module shaped the way `materializeMemoryImports` leaves a
// linkable TU, so what is asserted is the linker's behaviour on its own
// intermediate form rather than on a shape invented here.
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

/// A linkable TU: one exported function, and the `env.__*` runtime-state
/// imports the codegen emits for it -- including a `wvmcc_memidx(2)`
/// placement, which is the case that first surfaced this.
WasmVM::WasmModule linkableModule(const char* exportName) {
    WasmVM::WasmModule m;
    WasmVM::FuncType ft;
    ft.results.push_back(WasmVM::ValueType::i32);
    m.types.push_back(ft);

    const auto memImport = [&](const char* name) {
        WasmVM::WasmImport imp;
        imp.module = "env";
        imp.name = name;
        WasmVM::MemType mt;
        mt.min = 1;
        mt.is64 = true;
        imp.desc = mt;
        m.imports.push_back(imp);
    };
    const auto globalImport = [&](const char* name, bool mutable_) {
        WasmVM::WasmImport imp;
        imp.module = "env";
        imp.name = name;
        imp.desc = WasmVM::GlobalType{mutable_ ? WasmVM::GlobalType::variable
                                               : WasmVM::GlobalType::constant,
                                      WasmVM::ValueType::i64};
        m.imports.push_back(imp);
    };
    memImport("__linear_memory");
    memImport("__stack_memory");
    memImport("__memory_2");
    globalImport("__stack_pointer", true);
    globalImport("__heap_base", false);

    WasmVM::WasmFunc fn;
    fn.typeidx = 0;
    fn.body.push_back(WasmVM::Instr::I32_const{42});
    fn.body.push_back(WasmVM::Instr::End{});
    m.funcs.push_back(fn);

    WasmVM::WasmExport ex;
    ex.name = exportName;
    ex.desc = WasmVM::WasmExport::DescType::func;
    ex.index = 0;
    m.exports.push_back(ex);
    return m;
}

wvmcc::link::LinkResult linkOne(const WasmVM::WasmModule& m, bool noStdlib) {
    using namespace wvmcc::link;
    LinkInput::InMemoryModule mm{m, "test.c"};
    LinkInput in;
    in.source = std::move(mm);
    std::vector<LinkInput> inputs;
    inputs.push_back(std::move(in));
    LinkOptions opts;
    opts.no_stdlib = noStdlib;
    return link(std::move(inputs), opts);
}

bool hasImport(const WasmVM::WasmModule& m, const char* module, const char* name) {
    for (const auto& imp : m.imports) {
        if (imp.module == module && imp.name == name) return true;
    }
    return false;
}

bool hasExport(const WasmVM::WasmModule& m, const char* name) {
    for (const auto& ex : m.exports) {
        if (ex.name == name) return true;
    }
    return false;
}

std::size_t importsFromModule(const WasmVM::WasmModule& m, const char* module) {
    std::size_t count = 0;
    for (const auto& imp : m.imports) {
        if (imp.module == module) ++count;
    }
    return count;
}

} // namespace

int main() {
    // ---- The bug: `-nostdlib` left the env.__* imports in place -----------
    {
        auto r = linkOne(linkableModule("run"), /*noStdlib=*/true);
        EXPECT(r.ok, "-nostdlib link succeeds");

        // Steps 1-2 must run: no program can define mem[0] for itself.
        EXPECT(importsFromModule(r.module, "env") == 0,
               "-nostdlib drops every env.__* import");
        EXPECT(!hasImport(r.module, "env", "__linear_memory"), "no env.__linear_memory");
        EXPECT(!hasImport(r.module, "env", "__memory_2"), "no env.__memory_N");
        EXPECT(!hasImport(r.module, "env", "__stack_pointer"), "no env.__stack_pointer");

        // Index spaces preserved: a wvmcc_memidx(2) placement still lands at
        // mem[2], so no instruction rewriting is needed.
        EXPECT(r.module.mems.size() == 3,
               "three local memories: heap, stack, and the memidx(2) placement");
        EXPECT(r.module.globals.size() == 2, "stack pointer and heap base defined locally");

        // Steps 3-4 must NOT run: that is what the flag means.
        EXPECT(importsFromModule(r.module, "sys_proc") == 0,
               "-nostdlib imports no process ABI");
        EXPECT(!r.module.start.has_value(), "-nostdlib emits no start section");
        EXPECT(hasExport(r.module, "run"), "the module's own export survives");
    }

    // ---- The cut is below the `main`-export removal, not above it ---------
    //
    // That removal exists only so the start wrapper's own export does not
    // collide. Running it without the wrapper strips a `-nostdlib` module's
    // `main` export and gives nothing back -- which is what happens if the
    // early return is placed at the "step 3" comment instead.
    {
        auto r = linkOne(linkableModule("main"), /*noStdlib=*/true);
        EXPECT(r.ok, "-nostdlib link with a main succeeds");
        EXPECT(hasExport(r.module, "main"),
               "-nostdlib keeps the main export -- nothing re-adds it");
        EXPECT(!r.module.start.has_value(), "still no start section");
    }

    // ---- The default path is unchanged ------------------------------------
    {
        auto r = linkOne(linkableModule("main"), /*noStdlib=*/false);
        EXPECT(r.ok, "default link succeeds");
        EXPECT(importsFromModule(r.module, "env") == 0, "crt0 still drops env.__*");
        EXPECT(importsFromModule(r.module, "sys_proc") == 4,
               "crt0 still prepends the four sys_proc imports");
        EXPECT(r.module.start.has_value(), "crt0 still emits a start section");
        EXPECT(hasExport(r.module, "main"), "the start wrapper re-adds the main export");
    }

    if (failures == 0) {
        std::cout << "all crt0 -nostdlib tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
