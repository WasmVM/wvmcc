// Unit test: GCC __attribute__((...)) parsing on extern declarations
#include <iostream>
#include <fstream>
#include <cstdio>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

int main() {
    const std::string fname = "temp_gnu_attribute_test.c";
    {
        std::ofstream ofs(fname);
        // Two attributes in a single specifier list, between specifiers and declarator
        ofs << "extern int __attribute__((import_module(\"sys_proc\"), import_name(\"argc\"))) proc_argc(void);\n";
        // Attribute before declaration-specifiers
        ofs << "__attribute__((unused)) extern int marker_a;\n";
        // Attribute after the declarator
        ofs << "extern int marker_b __attribute__((import_module(\"sys_fs\"), import_name(\"write\")));\n";
        // Multiple separate __attribute__ specifiers should accumulate
        ofs << "extern int __attribute__((import_module(\"m1\"))) __attribute__((import_name(\"n1\"))) marker_c;\n";
        // Unknown attribute name with no args - silently retained
        ofs << "extern int __attribute__((something_unknown)) marker_d;\n";
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    std::remove(fname.c_str());
    if (!tu) return 3;

    if (tu->externals.size() != 5) {
        std::cerr << "expected 5 externals, got " << tu->externals.size() << "\n";
        return 4;
    }

    auto getDecl = [&](size_t i) -> DeclarationPtr {
        auto &ext = tu->externals[i];
        if (!std::holds_alternative<DeclarationPtr>(ext->decl)) return nullptr;
        return std::get<DeclarationPtr>(ext->decl);
    };

    // 0: proc_argc with two attributes between specs and declarator
    {
        auto d = getDecl(0);
        if (!d) return 10;
        if (d->gnuAttributes.size() != 2) {
            std::cerr << "expected 2 attrs on proc_argc, got " << d->gnuAttributes.size() << "\n";
            return 11;
        }
        if (d->gnuAttributes[0].name != "import_module") return 12;
        if (d->gnuAttributes[0].stringArgs.size() != 1) return 13;
        if (d->gnuAttributes[0].stringArgs[0] != "sys_proc") {
            std::cerr << "expected sys_proc, got " << d->gnuAttributes[0].stringArgs[0] << "\n";
            return 14;
        }
        if (d->gnuAttributes[1].name != "import_name") return 15;
        if (d->gnuAttributes[1].stringArgs.size() != 1) return 16;
        if (d->gnuAttributes[1].stringArgs[0] != "argc") return 17;
    }

    // 1: marker_a, attribute before specifiers
    {
        auto d = getDecl(1);
        if (!d) return 20;
        if (d->gnuAttributes.size() != 1) return 21;
        if (d->gnuAttributes[0].name != "unused") return 22;
    }

    // 2: marker_b, attribute after declarator
    {
        auto d = getDecl(2);
        if (!d) return 30;
        if (d->gnuAttributes.size() != 2) return 31;
        if (d->gnuAttributes[0].name != "import_module") return 32;
        if (d->gnuAttributes[0].stringArgs.empty() || d->gnuAttributes[0].stringArgs[0] != "sys_fs") return 33;
        if (d->gnuAttributes[1].name != "import_name") return 34;
        if (d->gnuAttributes[1].stringArgs.empty() || d->gnuAttributes[1].stringArgs[0] != "write") return 35;
    }

    // 3: marker_c, two separate __attribute__ specifiers should be flattened
    {
        auto d = getDecl(3);
        if (!d) return 40;
        if (d->gnuAttributes.size() != 2) {
            std::cerr << "marker_c attrs=" << d->gnuAttributes.size() << "\n";
            return 41;
        }
        if (d->gnuAttributes[0].name != "import_module") return 42;
        if (d->gnuAttributes[0].stringArgs.empty() || d->gnuAttributes[0].stringArgs[0] != "m1") return 43;
        if (d->gnuAttributes[1].name != "import_name") return 44;
        if (d->gnuAttributes[1].stringArgs.empty() || d->gnuAttributes[1].stringArgs[0] != "n1") return 45;
    }

    // 4: marker_d, unknown attribute name retained
    {
        auto d = getDecl(4);
        if (!d) return 50;
        if (d->gnuAttributes.size() != 1) return 51;
        if (d->gnuAttributes[0].name != "something_unknown") return 52;
        if (!d->gnuAttributes[0].stringArgs.empty()) return 53;
    }

    return 0;
}
