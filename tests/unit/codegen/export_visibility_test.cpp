// M2-C: __attribute__((visibility("default"))) opts a function into being
// exported under its C identifier; __attribute__((export_name("name")))
// exports under a custom name. `static` functions stay internal.
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
    codegen.setCompileMode(CompileMode::Linkable);
    auto m = codegen.generate(tu);
    std::remove(fname.c_str());
    return m;
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
    // visibility("default") — exported under the C identifier.
    {
        auto m = compile_linkable(
            "int __attribute__((visibility(\"default\"))) api_fn(int x) { return x; }\n"
            "int helper(int x) { return x + 1; }\n"
            "static int internal(int x) { return x * 2; }\n",
            "tmp_m2c_default.c");
        EXPECT(has_export(m, "api_fn"),
               "visibility(\"default\") exports api_fn");
        EXPECT(!has_export(m, "helper"),
               "no attribute, no export (linkable mode)");
        EXPECT(!has_export(m, "internal"),
               "static function never exported");
    }

    // export_name("custom") — exported under custom name.
    {
        auto m = compile_linkable(
            "int __attribute__((export_name(\"custom_name\"))) foo(int x) { return x; }\n",
            "tmp_m2c_export_name.c");
        EXPECT(has_export(m, "custom_name"),
               "export_name(\"custom_name\") emits export under custom_name");
        EXPECT(!has_export(m, "foo"),
               "export_name overrides — foo (C name) is NOT exported");
    }

    // static + visibility("default") — static wins.
    {
        auto m = compile_linkable(
            "static int __attribute__((visibility(\"default\"))) only_static(int x) { return x; }\n",
            "tmp_m2c_static.c");
        EXPECT(!has_export(m, "only_static"),
               "static function ignores visibility attribute");
    }

    if (failures == 0) {
        std::cout << "all export-visibility tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
