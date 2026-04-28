#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "parser/Semantic.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

static int run_case(const std::string &src, const std::string &expect_substr) {
    const std::string fname = "temp_sema_external_decl.c";
    {
        std::ofstream ofs(fname);
        ofs << src;
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    // run semantic pass to append diagnostics
    wvmcc::parser::Semantic sem(tu, false);
    bool sem_ok = sem.run(parser.getDiagnosticsRef()); (void)sem_ok;
    const auto &diags = parser.getDiagnostics();
    bool found = false;
    for (const auto &d : diags) {
        if (d.message.find(expect_substr) != std::string::npos) { found = true; break; }
    }
    if (!found) {
        std::cerr << "--- diagnostics for case (source):\n" << src << "\n---\n";
        for (const auto &d : diags) {
            std::cerr << (d.severity==wvmcc::Diagnostic::Severity::Error?"error: ":"note: ") << d.message;
            if (d.span.has_value()) {
                std::cerr << " (line " << d.span->begin.line << ")";
            }
            std::cerr << "\n";
        }
    }
    std::remove(fname.c_str());
    return found ? 0 : 3;
}

int main() {
    // Cases: storage-class in external decl, duplicate static function, two definitive static objects
    struct Case { std::string src; std::string expect; } cases[] = {
        { "auto x = 1;\n", "storage-class specifier" },
        { "static int f() { return 1; }\nstatic int f() { return 2; }\n", "duplicate internal definition" },
        { "static int x = 1;\nstatic int x = 2;\n", "duplicate internal definition" },
        { "static int x = foo;\n", "must be constant expression" },
        { "void f() { extern int x = 1; }\n", "block scope" },
        { "int x = 0;\nint arr[] = { [x] = 1 };\n", "designator index must be an integer constant expression" }
    };

    for (auto &c : cases) {
        int r = run_case(c.src, c.expect);
        if (r != 0) {
            std::cerr << "case failed for expect='" << c.expect << "'\n";
            return r;
        }
    }

    std::cout << "sema-external-decl: OK" << std::endl;
    return 0;
}
