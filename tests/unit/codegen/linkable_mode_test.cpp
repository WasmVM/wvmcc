// M2-D: ModuleCodegen in linkable mode imports runtime state from `env`
// instead of defining it locally, and skips the freestanding start wrapper.
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <variant>

#include <WasmVM.hpp>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "parser/Semantic.hpp"
#include "codegen/ModuleCodegen.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;
using namespace wvmcc::codegen;

static int failures = 0;

#define EXPECT(cond, msg)                                                     \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__,         \
                         __LINE__);                                           \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

static WasmVM::WasmModule compile_linkable(const std::string& src,
                                           const std::string& fname) {
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
    // Default is Linkable in M2-D; set explicitly for clarity.
    codegen.setCompileMode(CompileMode::Linkable);
    auto m = codegen.generate(tu);
    std::remove(fname.c_str());
    return m;
}

static bool has_env_import(const WasmVM::WasmModule& m, const std::string& name) {
    for (const auto& imp : m.imports) {
        if (imp.module == "env" && imp.name == name) return true;
    }
    return false;
}

static bool has_any_import_from(const WasmVM::WasmModule& m, const std::string& mod) {
    for (const auto& imp : m.imports) {
        if (imp.module == mod) return true;
    }
    return false;
}

int main() {
    // Trivial program — linkable mode should still import all the runtime
    // state even without an indirect call (table import requires &funcname).
    auto m = compile_linkable("int main(void) { return 0; }",
                              "tmp_linkable_simple.c");

    EXPECT(has_env_import(m, "__linear_memory"),
           "linkable mode imports env.__linear_memory");
    EXPECT(has_env_import(m, "__stack_memory"),
           "linkable mode imports env.__stack_memory");
    EXPECT(has_env_import(m, "__stack_pointer"),
           "linkable mode imports env.__stack_pointer");
    EXPECT(m.mems.empty(),
           "linkable mode defines no local memories");
    EXPECT(m.globals.empty(),
           "linkable mode defines no local globals");
    EXPECT(!has_any_import_from(m, "sys_proc"),
           "linkable mode does NOT inject sys_proc imports");
    EXPECT(!m.start.has_value(),
           "linkable mode emits no start section (linker provides crt0)");

    // Program that takes a function pointer — linkable mode imports the
    // shared funcref table from env.
    auto m2 = compile_linkable(
        "int helper(int x) { return x + 1; }\n"
        "int main(void) { int (*p)(int) = &helper; return p(3); }\n",
        "tmp_linkable_funcptr.c");
    EXPECT(has_env_import(m2, "__indirect_function_table"),
           "linkable mode imports env.__indirect_function_table");
    EXPECT(m2.tables.empty(),
           "linkable mode defines no local table when imports are present");

    if (failures == 0) {
        std::cout << "all linkable-mode tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
