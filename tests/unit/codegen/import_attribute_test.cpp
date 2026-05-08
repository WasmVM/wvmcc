// Issue #39: registerFunctionDeclaration must respect __attribute__((import_module, import_name))
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

static WasmVM::WasmModule compile(const std::string& src) {
    const std::string fname = "temp_import_attribute_test.c";
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
    auto module = codegen.generate(tu);
    std::remove(fname.c_str());
    return module;
}

// Find the first import whose name matches `name` (and optionally module).
// Used to skip past auto-injected sys_proc imports when main() is present.
static const WasmVM::WasmImport* findImport(const WasmVM::WasmModule& m,
                                            const std::string& name) {
    for (const auto& imp : m.imports) {
        if (imp.name == name) return &imp;
    }
    return nullptr;
}

int main() {
    // Acceptance from issue #39: extern function with import_module + import_name.
    // Use a non-main translation unit so we don't trip the issue-#40 sys_proc
    // auto-injection — that path is covered by start_wrapper_test.
    {
        auto m = compile(
            "extern int __attribute__((import_module(\"sys_proc\"), "
            "import_name(\"argc\"))) proc_argc(void);\n"
            "int caller(void) { return proc_argc(); }\n");
        if (m.imports.empty()) {
            std::cerr << "expected at least one import\n"; return 2;
        }
        const auto& imp = m.imports[0];
        if (imp.module != "sys_proc") {
            std::cerr << "expected module='sys_proc', got '" << imp.module << "'\n"; return 3;
        }
        if (imp.name != "argc") {
            std::cerr << "expected name='argc', got '" << imp.name << "'\n"; return 4;
        }
    }

    // Regression: extern without attributes still falls back to module="env" and C name.
    {
        auto m = compile(
            "extern int plain_extern(int);\n"
            "int caller(int x) { return plain_extern(x); }\n");
        const auto* imp = findImport(m, "plain_extern");
        if (!imp) { std::cerr << "expected import for plain_extern\n"; return 5; }
        if (imp->module != "env") {
            std::cerr << "expected fallback module='env', got '" << imp->module << "'\n"; return 6;
        }
    }

    // Only import_module specified: import_name should default to the C identifier.
    {
        auto m = compile(
            "extern int __attribute__((import_module(\"custom\"))) only_module(void);\n"
            "int caller(void) { return only_module(); }\n");
        const auto* imp = findImport(m, "only_module");
        if (!imp) return 8;
        if (imp->module != "custom") return 9;
    }

    // Auto-injection coexists with attributed imports: with main() defined the
    // four sys_proc imports come first, then the user's imports are appended.
    {
        auto m = compile(
            "extern int __attribute__((import_module(\"custom\"), "
            "import_name(\"hello\"))) my_func(void);\n"
            "int main(void) { return my_func(); }\n");
        if (m.imports.size() < 5) {
            std::cerr << "expected 5 imports, got " << m.imports.size() << "\n";
            return 11;
        }
        if (m.imports[0].module != "sys_proc") return 12;
        const auto* imp = findImport(m, "hello");
        if (!imp || imp->module != "custom") return 13;
    }

    std::cout << "import_attribute_test passed\n";
    return 0;
}
