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
    const std::string fname = "temp_sema_static.c";
    {
        std::ofstream ofs(fname);
        ofs << src;
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    wvmcc::parser::Semantic sem(tu, false);
    bool sem_ok = sem.run(parser.getDiagnosticsRef()); (void)sem_ok;
    const auto &diags = parser.getDiagnostics();
    bool found = false;
    if (expect_substr.empty()) {
        // expect no diagnostics
        if (!diags.empty()) {
            std::cerr << "unexpected diagnostics for case (source):\n" << src << "\n---\n";
            for (const auto &d : diags) {
                std::cerr << (d.severity==wvmcc::Diagnostic::Severity::Error?"error: ":"note: ") << d.message;
                if (d.span.has_value()) std::cerr << " (line " << d.span->begin.line << ")";
                std::cerr << "\n";
            }
            std::remove(fname.c_str());
            return 3;
        }
        std::remove(fname.c_str());
        return 0;
    }
    for (const auto &d : diags) {
        if (d.message.find(expect_substr) != std::string::npos) { found = true; break; }
    }
    if (!found) {
        std::cerr << "--- diagnostics for case (source):\n" << src << "\n---\n";
        for (const auto &d : diags) {
            std::cerr << (d.severity==wvmcc::Diagnostic::Severity::Error?"error: ":"note: ") << d.message;
            if (d.span.has_value()) std::cerr << " (line " << d.span->begin.line << ")";
            std::cerr << "\n";
        }
    }
    std::remove(fname.c_str());
    return found ? 0 : 3;
}

int main() {
    struct Case { std::string src; std::string expect; } cases[] = {
        { "_Static_assert(1, \"ok\");\n", "" },
        { "_Static_assert(0, \"failed message\");\n", "failed message" },
        { "int x = 1; _Static_assert(x, \"non-const\");\n", "_Static_assert requires an integer constant expression" }
    };

    for (auto &c : cases) {
        int r = run_case(c.src, c.expect);
        if (r != 0) {
            std::cerr << "case failed for expect='" << c.expect << "'\n";
            return r;
        }
    }

    std::cout << "sema-static-assert: OK" << std::endl;
    return 0;
}
