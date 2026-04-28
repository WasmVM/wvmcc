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

// run_case: expects the LAST external declaration to be a Declaration with an initializer
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
    if (!tu || tu->externals.empty()) { std::remove(fname.c_str()); return 3; }

    // check for parse errors
    for (const auto &d : parser.getDiagnostics()) {
        if (d.severity == wvmcc::Diagnostic::Severity::Error) {
            std::cerr << "parse error: " << d.message << "\n";
            std::remove(fname.c_str());
            return 8;
        }
    }

    // the last external declaration should be a Declaration with an initializer
    auto ext = tu->externals.back();
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

    // .member designated initializer (C17 §6.7.9)
    if (run_case("struct Point { int x; int y; }; struct Point p = { .x = 1, .y = 2 };\n") != 0) { std::cerr << ".member designated initializer failed" << std::endl; return 5; }

    // mixed .member and [n] designated initializers
    if (run_case("struct S { int a; int b; }; struct S s = { .b = 10, .a = 20 };\n") != 0) { std::cerr << "mixed member designated initializer failed" << std::endl; return 6; }

    // .member with nested struct
    if (run_case("struct Inner { int v; }; struct Outer { struct Inner i; int n; }; struct Outer o = { .i = {1}, .n = 2 };\n") != 0) { std::cerr << "nested struct member designated initializer failed" << std::endl; return 7; }

    std::cout << "initializer-test: OK" << std::endl;
    return 0;
}
