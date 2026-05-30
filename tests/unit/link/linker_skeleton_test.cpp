// M2-L1: the link skeleton is callable, runs the phase pipeline, and for
// the single-in-memory-module case produces output == input (pass-through).
// Real merge / DCE / crt0 land in subsequent issues.
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

int main() {
    using namespace wvmcc::link;

    // Build a trivial WasmModule by hand: one type, one function, one export.
    WasmVM::WasmModule m;
    WasmVM::FuncType ft;
    ft.results.push_back(WasmVM::ValueType::i32);
    m.types.push_back(ft);

    WasmVM::WasmFunc fn;
    fn.typeidx = 0;
    fn.body.push_back(WasmVM::Instr::I32_const{42});
    fn.body.push_back(WasmVM::Instr::End{});
    m.funcs.push_back(fn);

    WasmVM::WasmExport ex;
    ex.name = "answer";
    ex.desc = WasmVM::WasmExport::DescType::func;
    ex.index = 0;
    m.exports.push_back(ex);

    // Single in-memory input → pass-through.
    {
        LinkInput::InMemoryModule mm{m, "test.c"};
        LinkInput in;
        in.source = std::move(mm);

        std::vector<LinkInput> inputs;
        inputs.push_back(std::move(in));

        LinkOptions opts;
        opts.verbose = true;
        opts.no_stdlib = true; // mini module has no `main`; skip crt0.
        auto r = link(std::move(inputs), opts);

        EXPECT(r.ok, "single-module link succeeds");
        EXPECT(r.module.funcs.size() == 1, "output has the one function");
        EXPECT(r.module.exports.size() == 1, "output has the one export");
        EXPECT(r.module.exports[0].name == "answer", "export name preserved");

        // Verbose log should mention every phase.
        std::string joined;
        for (const auto& l : r.log) joined += l + "\n";
        EXPECT(joined.find("phase: merge")          != std::string::npos, "logs merge phase");
        EXPECT(joined.find("phase: indirect-table") != std::string::npos, "logs indirect-table phase");
        EXPECT(joined.find("phase: reloc-apply")    != std::string::npos, "logs reloc-apply phase");
        EXPECT(joined.find("phase: crt0-synth")     != std::string::npos, "logs crt0-synth phase");
        EXPECT(joined.find("phase: import-resol")   != std::string::npos, "logs import-resolution phase");
        EXPECT(joined.find("phase: dce")            != std::string::npos, "logs dce phase");
        EXPECT(joined.find("phase: diagnostics")    != std::string::npos, "logs diagnostics phase");
    }

    // Archive input fails today (M2-L4 deferred).
    {
        LinkInput::ArchivePath ap{"/nonexistent/libc.a"};
        LinkInput in;
        in.source = ap;
        std::vector<LinkInput> inputs;
        inputs.push_back(std::move(in));
        auto r = link(std::move(inputs), LinkOptions{});
        EXPECT(!r.ok, "archive input fails (M2-L4 deferred)");
    }

    // Multi-module link succeeds (M2-L2): two modules with the same single
    // export name should collide.
    {
        LinkInput::InMemoryModule a{m, "a.c"};
        LinkInput::InMemoryModule b{m, "b.c"};
        LinkInput ia, ib;
        ia.source = std::move(a);
        ib.source = std::move(b);
        std::vector<LinkInput> inputs;
        inputs.push_back(std::move(ia));
        inputs.push_back(std::move(ib));
        LinkOptions o;
        o.no_stdlib = true;
        auto r = link(std::move(inputs), o);
        EXPECT(!r.ok, "duplicate export 'answer' across modules → error");
    }

    if (failures == 0) {
        std::cout << "all linker-skeleton tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
