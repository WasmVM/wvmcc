#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

static int run_case(const std::string &src, const std::string &expect_substr, bool expect_diag) {
    const std::string fname = "temp_static_assert_test.c";
    {
        std::ofstream ofs(fname);
        ofs << src;
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    (void)parser.parseTranslationUnit();
    const auto &diags = parser.getDiagnostics();
    bool found = false;
    for (const auto &d : diags) {
        if (d.message.find(expect_substr) != std::string::npos) { found = true; break; }
    }
    std::remove(fname.c_str());
    if (expect_diag) return found ? 0 : 3;
    return (!expect_diag && diags.empty()) ? 0 : 4;
}

int main() {
    // successful static assert
    if (run_case("_Static_assert(1, \"ok\");\n", "ok", false) != 0) { std::cerr << "static assert success case failed" << std::endl; return 1; }

    // failing static assert
    if (run_case("_Static_assert(0, \"failed\");\n", "static assertion failed", true) != 0) { std::cerr << "static assert failure case failed" << std::endl; return 2; }

    // non-constant expression
    if (run_case("int x = 1; _Static_assert(x, \"nope\");\n", "requires an integer constant expression", true) != 0) { std::cerr << "static assert non-const case failed" << std::endl; return 3; }

    std::cout << "static-assert-test: OK" << std::endl;
    return 0;
}
