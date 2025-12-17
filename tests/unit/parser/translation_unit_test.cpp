// Unit test: parse a simple translation unit and verify externals
#include <iostream>
#include <cassert>
#include <sstream>
#include <fstream>
#include <cstdio>
#include "../../src/pp/Preprocessor.hpp"
#include "../../src/parser/Lexer.hpp"
#include "../../src/parser/Parser.hpp"

int main() {
    using namespace wvmcc;
    using namespace wvmcc::parser;

    const std::string srcName = "temp_translation_unit_test.c";
    {
        std::ofstream ofs(srcName);
        ofs << "int add(int a, int b) { return a + b; }\n\n";
        ofs << "int main() {\n    int x = add(2, 3);\n    return x;\n}\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) {
        std::remove(srcName.c_str());
        std::cerr << "preprocessor open failed" << std::endl;
        return 2;
    }

    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    if (!tu) {
        std::cerr << "parse failed" << std::endl;
        return 3;
    }

    // Expect two external declarations (add, main)
    if (tu->externals.size() != 2) {
        std::cerr << "expected 2 externals, got " << tu->externals.size() << std::endl;
        return 4;
    }

    // Both externals should be function definitions in this simple test
    for (size_t i = 0; i < tu->externals.size(); ++i) {
        auto &ext = tu->externals[i];
        if (!std::holds_alternative<FunctionDefPtr>(ext->decl)) {
            std::cerr << "external[" << i << "] is not a FunctionDef" << std::endl;
            return 5;
        }
    }

    std::cout << "translation-unit: OK" << std::endl;
    std::remove(srcName.c_str());
    return 0;
}
