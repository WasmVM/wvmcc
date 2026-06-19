// Multi-memory placement: __attribute__((wvmcc_memidx(N))) puts a file-scope
// object in linear memory N (2..14) instead of the default mem[0]. This test
// inspects the emitted module structurally; the end-to-end runtime behavior
// (load/store/deref dispatch under wasmvm) is exercised separately.
//
// Checked here, in freestanding mode (self-contained — own memories):
//   - the requested memory is created (module gains mem[2]/mem[3])
//   - the object's initializer data segment targets that memory
//   - a named access compiles to a load against (memory N)
//   - a plain global is unaffected (stays in mem[0], no extra memory)
//   - an out-of-range index is rejected with a diagnostic
#include <iostream>
#include <fstream>
#include <cstdio>
#include <WasmVM.hpp>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "parser/Semantic.hpp"
#include "codegen/ModuleCodegen.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;
using namespace wvmcc::codegen;

struct CompileResult {
    WasmVM::WasmModule module;
    std::vector<wvmcc::Diagnostic> diags;
};

static CompileResult compileMode(const std::string& src, CompileMode mode) {
    const std::string fname = "temp_multimem_attribute_test.c";
    {
        std::ofstream ofs(fname);
        ofs << src;
    }
    Preprocessor pp;
    pp.open(fname);
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    Semantic sem(tu, false);
    sem.run(parser.getDiagnosticsRef());
    ModuleCodegen codegen(sem);
    codegen.setCompileMode(mode);
    auto module = codegen.generate(tu);
    std::remove(fname.c_str());
    return {std::move(module), codegen.getDiagnostics()};
}

static CompileResult compile(const std::string& src) {
    return compileMode(src, CompileMode::Freestanding);
}

// True if the module imports a memory named env.<name>.
static bool importsEnvMemory(const WasmVM::WasmModule& m, const std::string& name) {
    for (const auto& imp : m.imports) {
        if (imp.module == "env" && imp.name == name &&
            std::holds_alternative<WasmVM::MemType>(imp.desc)) return true;
    }
    return false;
}

// True if any data segment is active in `memidx`.
static bool hasDataSegInMemory(const WasmVM::WasmModule& m, WasmVM::index_t memidx) {
    for (const auto& d : m.datas) {
        if (d.mode.memidx.has_value() && *d.mode.memidx == memidx) return true;
    }
    return false;
}

// True if any function body contains a memory-access instruction (load/store)
// whose memarg targets `memidx`.
static bool hasMemAccessInMemory(const WasmVM::WasmModule& m, WasmVM::index_t memidx) {
    for (const auto& f : m.funcs) {
        for (const auto& instr : f.body) {
            if (auto* ma = std::get_if<WasmVM::WasmInstr::MemArg>(&instr.imm)) {
                if (ma->memidx == memidx) return true;
            }
        }
    }
    return false;
}

static bool hasError(const std::vector<wvmcc::Diagnostic>& diags) {
    for (const auto& d : diags) {
        if (d.severity == wvmcc::Diagnostic::Severity::Error) return true;
    }
    return false;
}

int main() {
    // Two placements: mem[2] scalar (read by a function) and mem[3] array.
    {
        auto r = compile(
            "__attribute__((wvmcc_memidx(2))) int a = 42;\n"
            "__attribute__((wvmcc_memidx(3))) int arr[3] = {1, 2, 3};\n"
            "int rd(void) { return a; }\n");
        if (hasError(r.diags)) {
            std::cerr << "unexpected diagnostic for valid placement\n"; return 2;
        }
        // mem[0], mem[1] always exist; mem[2] and mem[3] were requested.
        if (r.module.mems.size() != 4) {
            std::cerr << "expected 4 memories, got " << r.module.mems.size() << "\n"; return 3;
        }
        if (!hasDataSegInMemory(r.module, 2)) {
            std::cerr << "expected an initializer data segment in mem[2]\n"; return 4;
        }
        if (!hasDataSegInMemory(r.module, 3)) {
            std::cerr << "expected an initializer data segment in mem[3]\n"; return 5;
        }
        // rd() loads `a` directly from (memory 2).
        if (!hasMemAccessInMemory(r.module, 2)) {
            std::cerr << "expected a named load/store against mem[2]\n"; return 6;
        }
    }

    // Regression: an unattributed global stays in mem[0]; no extra memory.
    {
        auto r = compile(
            "int g = 7;\n"
            "int rd(void) { return g; }\n");
        if (r.module.mems.size() != 2) {
            std::cerr << "plain global should not create extra memories, got "
                      << r.module.mems.size() << "\n"; return 7;
        }
        if (hasDataSegInMemory(r.module, 2)) {
            std::cerr << "plain global must not land in mem[2]\n"; return 8;
        }
    }

    // Out-of-range index (15 is the function-pointer tag) is rejected.
    {
        auto r = compile("__attribute__((wvmcc_memidx(15))) int bad = 1;\n");
        if (!hasError(r.diags)) {
            std::cerr << "expected a diagnostic for wvmcc_memidx(15)\n"; return 9;
        }
    }

    // Reserved low index (1 = shadow stack) is rejected too.
    {
        auto r = compile("__attribute__((wvmcc_memidx(1))) int bad = 1;\n");
        if (!hasError(r.diags)) {
            std::cerr << "expected a diagnostic for wvmcc_memidx(1)\n"; return 10;
        }
    }

    // Linkable mode (the default, multi-TU): the object must *import*
    // env.__memory_N for each placement (the linker's crt0 makes them local),
    // not define a local memory. The data segment still targets memory N.
    {
        auto r = compileMode(
            "__attribute__((wvmcc_memidx(2))) int a = 1;\n"
            "__attribute__((wvmcc_memidx(3))) int b = 2;\n"
            "int rd(void){ return a + b; }\n",
            CompileMode::Linkable);
        if (hasError(r.diags)) {
            std::cerr << "unexpected diagnostic for linkable placement\n"; return 11;
        }
        if (!importsEnvMemory(r.module, "__memory_2") ||
            !importsEnvMemory(r.module, "__memory_3")) {
            std::cerr << "linkable mode must import env.__memory_2/3\n"; return 12;
        }
        if (!hasDataSegInMemory(r.module, 2) || !hasDataSegInMemory(r.module, 3)) {
            std::cerr << "linkable data segments must target mem[2]/mem[3]\n"; return 13;
        }
    }

    std::cout << "multimem_attribute_test passed\n";
    return 0;
}
