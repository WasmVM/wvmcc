// M2-C: __attribute__((visibility("default"))) opts a function into being
// exported under its C identifier; __attribute__((export_name("name")))
// exports under a custom name. `static` functions stay internal.
//
// Export selectivity (only attributed functions exported) is the *freestanding*
// contract. In *linkable* mode the export section doubles as the linker's
// symbol table — the name-based import resolver (M2-L3) wires cross-TU calls by
// matching imports against exports — so every non-static function is exported
// under its C name. There is no separate wasm "linking" custom section in
// wvmcc, so this is intrinsic to the resolver design, not an accident. `static`
// is never exported in either mode; `export_name` always overrides the C name.
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

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
        if (!(cond)) { std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg,        \
                                    __FILE__, __LINE__); ++failures; }        \
    } while (0)

static WasmVM::WasmModule compile_mode(const std::string& src,
                                       const std::string& fname,
                                       CompileMode mode) {
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
    auto m = codegen.generate(tu);
    std::remove(fname.c_str());
    return m;
}

static WasmVM::WasmModule compile_freestanding(const std::string& src,
                                               const std::string& fname) {
    return compile_mode(src, fname, CompileMode::Freestanding);
}

static WasmVM::WasmModule compile_linkable(const std::string& src,
                                           const std::string& fname) {
    return compile_mode(src, fname, CompileMode::Linkable);
}

static bool has_export(const WasmVM::WasmModule& m, const std::string& name) {
    for (const auto& ex : m.exports) {
        if (ex.name == name && ex.desc == WasmVM::WasmExport::DescType::func) {
            return true;
        }
    }
    return false;
}

int main() {
    // --- Freestanding mode: selective export (only attributed functions). ---

    // visibility("default") — exported under the C identifier; a plain
    // function with no attribute stays internal.
    {
        auto m = compile_freestanding(
            "int __attribute__((visibility(\"default\"))) api_fn(int x) { return x; }\n"
            "int helper(int x) { return x + 1; }\n"
            "static int internal(int x) { return x * 2; }\n",
            "tmp_m2c_default.c");
        EXPECT(has_export(m, "api_fn"),
               "visibility(\"default\") exports api_fn (freestanding)");
        EXPECT(!has_export(m, "helper"),
               "no attribute, no export (freestanding)");
        EXPECT(!has_export(m, "internal"),
               "static function never exported (freestanding)");
    }

    // export_name("custom") — exported under custom name only.
    {
        auto m = compile_freestanding(
            "int __attribute__((export_name(\"custom_name\"))) foo(int x) { return x; }\n",
            "tmp_m2c_export_name.c");
        EXPECT(has_export(m, "custom_name"),
               "export_name(\"custom_name\") emits export under custom_name");
        EXPECT(!has_export(m, "foo"),
               "export_name overrides — foo (C name) is NOT exported");
    }

    // static + visibility("default") — static wins.
    {
        auto m = compile_freestanding(
            "static int __attribute__((visibility(\"default\"))) only_static(int x) { return x; }\n",
            "tmp_m2c_static.c");
        EXPECT(!has_export(m, "only_static"),
               "static function ignores visibility attribute (freestanding)");
    }

    // --- Linkable mode: export section is the linker's symbol table. ---

    // Every non-static function is exported under its C name so the name-based
    // resolver (M2-L3) can wire cross-TU calls; static stays internal.
    {
        auto m = compile_linkable(
            "int api_fn(int x) { return x; }\n"
            "int helper(int x) { return x + 1; }\n"
            "static int internal(int x) { return x * 2; }\n",
            "tmp_m2c_linkable.c");
        EXPECT(has_export(m, "api_fn"),
               "non-static function exported by C name (linkable)");
        EXPECT(has_export(m, "helper"),
               "non-static function exported by C name (linkable)");
        EXPECT(!has_export(m, "internal"),
               "static function never exported (linkable)");
    }

    // export_name still overrides the C name in linkable mode.
    {
        auto m = compile_linkable(
            "int __attribute__((export_name(\"custom_name\"))) foo(int x) { return x; }\n",
            "tmp_m2c_linkable_name.c");
        EXPECT(has_export(m, "custom_name"),
               "export_name overrides C name (linkable)");
        EXPECT(!has_export(m, "foo"),
               "export_name overrides — foo (C name) not exported (linkable)");
    }

    if (failures == 0) {
        std::cout << "all export-visibility tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
