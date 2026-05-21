// M2-E: in linkable mode, ModuleCodegen records every data-pointer
// i64.const as a relocation, exposes a data-symbol table, and the driver
// appends `linking` + `reloc.CODE` custom sections to the encoded file.
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <WasmVM.hpp>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "parser/Semantic.hpp"
#include "codegen/ModuleCodegen.hpp"
#include "codegen/RelocSection.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;
using namespace wvmcc::codegen;

static int failures = 0;
#define EXPECT(cond, msg)                                                     \
    do {                                                                      \
        if (!(cond)) { std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg,        \
                                    __FILE__, __LINE__); ++failures; }        \
    } while (0)

int main() {
    const std::string fname = "tmp_m2e.c";
    {
        std::ofstream ofs(fname);
        // Emit two string literals INSIDE a function so they actually go
        // through emitStringLiteral and get recorded as data-pointer sites.
        // (Global-scope initializers are still a stub in the current
        // emitGlobalScalar — see ModuleCodegen.cpp:539.)
        ofs << "void take(const char *p);\n"
               "int main(void) {\n"
               "    take(\"hello\");\n"
               "    take(\"world\");\n"
               "    return 0;\n"
               "}\n";
    }
    Preprocessor pp;
    pp.open(fname);
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    Semantic sem(tu, false);
    sem.run(parser.getDiagnosticsRef());
    ModuleCodegen cg(sem);
    cg.setCompileMode(CompileMode::Linkable);
    auto m = cg.generate(tu);
    std::remove(fname.c_str());

    EXPECT(cg.getDataSymbols().size() == 2,
           "two string literals → two data symbols");
    EXPECT(cg.getRelocations().size() == 2,
           "two i64.const data-pointer sites → two relocations");

    // Each relocation should reference a valid data symbol.
    for (const auto& r : cg.getRelocations()) {
        EXPECT(r.dataSymbolIdx < cg.getDataSymbols().size(),
               "relocation references an in-range data symbol");
        EXPECT(r.codeFuncIdx == 0,
               "both literal uses are in the single user function");
    }

    // Encode + append and verify the section names are present in the
    // output byte stream.
    std::ostringstream oss(std::ios::binary);
    WasmVM::module_encode(m, oss);
    appendRelocSections(oss, cg);
    std::string bytes = oss.str();
    EXPECT(bytes.find("linking") != std::string::npos,
           "encoded file contains 'linking' custom section name");
    EXPECT(bytes.find("reloc.CODE") != std::string::npos,
           "encoded file contains 'reloc.CODE' custom section name");

    // Freestanding mode emits nothing.
    {
        const std::string fs = "tmp_m2e_fs.c";
        {
            std::ofstream ofs(fs);
            ofs << "void take(const char *p);\n"
                   "int main(void) { take(\"hello\"); return 0; }\n";
        }
        Preprocessor pp2;
        pp2.open(fs);
        Lexer lex2(pp2);
        Parser parser2(lex2);
        auto tu2 = parser2.parseTranslationUnit();
        Semantic sem2(tu2, false);
        sem2.run(parser2.getDiagnosticsRef());
        ModuleCodegen cg2(sem2);
        cg2.setCompileMode(CompileMode::Freestanding);
        (void)cg2.generate(tu2);
        std::remove(fs.c_str());
        EXPECT(cg2.getRelocations().empty(),
               "freestanding mode records no relocations");
        EXPECT(cg2.getDataSymbols().empty(),
               "freestanding mode records no data symbols");
    }

    if (failures == 0) {
        std::cout << "all reloc-section tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
