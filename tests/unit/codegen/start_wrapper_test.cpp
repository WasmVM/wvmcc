// Issue #40: ModuleCodegen auto-injects sys_proc imports and emits a start
// wrapper for `main`. This test compiles small programs and checks the resulting
// WasmModule structure (imports, start, exports, validation).
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

static WasmVM::WasmModule compile(const std::string& src, const std::string& fname) {
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
    auto m = codegen.generate(tu);
    std::remove(fname.c_str());
    return m;
}

static int test_no_main_no_injection() {
    auto m = compile("int unused(int x) { return x + 1; }\n",
                     "temp_start_wrapper_no_main.c");
    // Without main, no sys_proc imports should be injected and no start section.
    if (!m.imports.empty()) {
        std::cerr << "expected no imports, got " << m.imports.size() << "\n";
        return 1;
    }
    if (m.start.has_value()) {
        std::cerr << "expected no start section\n"; return 2;
    }
    if (auto err = WasmVM::module_validate(m)) {
        std::cerr << "validation: " << err->what() << "\n"; return 3;
    }
    return 0;
}

static int test_main_void() {
    auto m = compile("int main(void) { return 7; }\n",
                     "temp_start_wrapper_main_void.c");
    // Four sys_proc imports at the front (argc, argv_len, argv, exit).
    if (m.imports.size() != 4) {
        std::cerr << "expected 4 imports, got " << m.imports.size() << "\n";
        return 1;
    }
    const char* expectedNames[4] = {"argc", "argv_len", "argv", "exit"};
    for (int i = 0; i < 4; ++i) {
        if (m.imports[i].module != "sys_proc") return 10 + i;
        if (m.imports[i].name != expectedNames[i]) {
            std::cerr << "import " << i << " name=" << m.imports[i].name << "\n";
            return 20 + i;
        }
    }
    // Start section points at the wrapper (the function after `main`).
    if (!m.start.has_value()) { std::cerr << "no start\n"; return 30; }
    auto wrapperIdx = (size_t)*m.start;
    if (wrapperIdx != m.imports.size() + m.funcs.size() - 1) {
        std::cerr << "wrong start idx: " << wrapperIdx << "\n"; return 31;
    }
    // main is exported by name with index = imports.size() (first user func).
    bool foundMain = false;
    for (const auto& ex : m.exports) {
        if (ex.name == "main" && ex.desc == WasmVM::WasmExport::DescType::func) {
            if (ex.index != (WasmVM::index_t)m.imports.size()) {
                std::cerr << "main export idx=" << ex.index << "\n"; return 32;
            }
            foundMain = true;
            break;
        }
    }
    if (!foundMain) { std::cerr << "no main export\n"; return 33; }
    // The wrapper itself: type should be () -> ().
    const auto& wrapperFunc = m.funcs.back();
    const auto& wrapperType = m.types[wrapperFunc.typeidx];
    if (!wrapperType.params.empty()) return 40;
    if (!wrapperType.results.empty()) return 41;
    if (auto err = WasmVM::module_validate(m)) {
        std::cerr << "validation failed: " << err->what() << "\n"; return 50;
    }
    return 0;
}

static int test_main_argv_funcType() {
    auto m = compile("int main(int argc, char **argv) { return argc - 1; }\n",
                     "temp_start_wrapper_main_argv.c");
    if (m.imports.size() != 4) return 1;
    if (!m.start.has_value()) return 2;
    // main is defined as the first user function at index = imports.size()
    auto mainIdx = m.imports.size();
    // Look up its FuncType — should be (i32, i64) -> i32.
    const auto& mainFunc = m.funcs[0];
    const auto& mainType = m.types[mainFunc.typeidx];
    if (mainType.params.size() != 2) {
        std::cerr << "main params=" << mainType.params.size() << "\n"; return 3;
    }
    if (mainType.params[0] != WasmVM::ValueType::i32) return 4;
    if (mainType.params[1] != WasmVM::ValueType::i64) return 5;
    if (mainType.results.size() != 1 || mainType.results[0] != WasmVM::ValueType::i32) return 6;
    // Wrapper has 5 locals (argc:i32, i:i32, len:i32, argv_base:i64, sp_save:i64)
    const auto& wrapperFunc = m.funcs.back();
    if (wrapperFunc.locals.size() != 5) {
        std::cerr << "wrapper locals=" << wrapperFunc.locals.size() << "\n"; return 7;
    }
    if (wrapperFunc.locals[0] != WasmVM::ValueType::i32) return 8;
    if (wrapperFunc.locals[3] != WasmVM::ValueType::i64) return 9;
    if (auto err = WasmVM::module_validate(m)) {
        std::cerr << "validation: " << err->what() << "\n"; return 10;
    }
    (void)mainIdx;
    return 0;
}

static int test_user_extern_after_sys_proc() {
    // A user-declared sys_proc import should still appear (after the four
    // auto-injected ones), with module/name driven by attributes.
    auto m = compile(
        "extern int __attribute__((import_module(\"sys_proc\"), "
        "import_name(\"argc\"))) my_argc(void);\n"
        "int main(void) { return my_argc(); }\n",
        "temp_start_wrapper_user_argc.c");
    if (m.imports.size() != 5) {
        std::cerr << "expected 5 imports, got " << m.imports.size() << "\n";
        return 1;
    }
    if (m.imports[0].name != "argc") return 2; // injected
    if (m.imports[4].module != "sys_proc" || m.imports[4].name != "argc") return 3;
    if (auto err = WasmVM::module_validate(m)) {
        std::cerr << "validation: " << err->what() << "\n"; return 4;
    }
    return 0;
}

#define RUN(fn) do { int r = fn(); if (r) { std::cerr << #fn " failed (" << r << ")\n"; return r; } std::cout << #fn " passed\n"; } while (0)

int main() {
    RUN(test_no_main_no_injection);
    RUN(test_main_void);
    RUN(test_main_argv_funcType);
    RUN(test_user_extern_after_sys_proc);
    std::cout << "start_wrapper_test passed\n";
    return 0;
}
