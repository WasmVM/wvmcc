#include <iostream>
#include <cassert>
#include <sstream>
#include <fstream>
#include <cstdio>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

static int run_case(const std::string &src) {
    const std::string fname = "temp_initializer_test.c";
    {
        std::ofstream ofs(fname);
        ofs << src;
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    if (!tu) { std::remove(fname.c_str()); return 3; }

    // expect a single external declaration which is a Declaration
    if (tu->externals.size() != 1) { std::remove(fname.c_str()); return 4; }
    auto ext = tu->externals[0];
    if (!std::holds_alternative<DeclarationPtr>(ext->decl)) { std::remove(fname.c_str()); return 5; }
    auto decl = std::get<DeclarationPtr>(ext->decl);
    if (!decl) { std::remove(fname.c_str()); return 6; }

    // ensure initializer present
    if (!decl->initializer.has_value()) { std::remove(fname.c_str()); return 7; }
    std::remove(fname.c_str());
    return 0;
}

int main() {
    // simple expression initializer
    if (run_case("int g = 42;\n") != 0) { std::cerr << "simple expr initializer failed" << std::endl; return 1; }

    // braced initializer-list
    if (run_case("int a[] = {1, 2, 3};\n") != 0) { std::cerr << "braced initializer-list failed" << std::endl; return 2; }

    // designated initializer with constant index
    if (run_case("int arr[4] = { [2] = 5 };\n") != 0) { std::cerr << "designated initializer failed" << std::endl; return 3; }

    // nested initializer lists
    if (run_case("int m[2][2] = { {1,2}, {3,4} };\n") != 0) { std::cerr << "nested initializer failed" << std::endl; return 4; }

    std::cout << "initializer-test: OK" << std::endl;
    return 0;
}
