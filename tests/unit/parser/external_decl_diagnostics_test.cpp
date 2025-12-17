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

static int run_case(const std::string &src, const std::string &expect_substr) {
    const std::string fname = "temp_external_decl_diag.c";
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
    return found ? 0 : 3;
}

int main() {
    // Cases: storage-class in external decl, duplicate static function, two definitive static objects
    struct Case { std::string src; std::string expect; } cases[] = {
        { "auto x = 1;\n", "storage-class specifier" },
        { "static int f() { return 1; }\nstatic int f() { return 2; }\n", "duplicate internal definition" },
        { "static int x = 1;\nstatic int x = 2;\n", "duplicate internal definition" }
    };

    for (auto &c : cases) {
        int r = run_case(c.src, c.expect);
        if (r != 0) {
            std::cerr << "case failed for expect='" << c.expect << "'\n";
            return r;
        }
    }

    std::cout << "external-decl-diagnostics: OK" << std::endl;
    return 0;
}
