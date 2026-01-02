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

static int run_case(const std::string &src, const std::string &expect_substr, bool expect_present) {
    const std::string fname = "temp_sema_qual.c";
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
    sem.run(parser.getDiagnosticsRef());
    const auto &diags = parser.getDiagnostics();
    bool found = false;
    for (const auto &d : diags) {
        if (d.message.find(expect_substr) != std::string::npos) { found = true; break; }
    }
    if (found != expect_present) {
        std::cerr << "--- diagnostics for case (source):\n" << src << "\n---\n";
        for (const auto &d : diags) {
            std::cerr << (d.severity==wvmcc::Diagnostic::Severity::Error?"error: ":"note: ") << d.message;
            if (d.span.has_value()) std::cerr << " (line " << d.span->begin.line << ")";
            std::cerr << "\n";
        }
    }
    std::remove(fname.c_str());
    return (found == expect_present) ? 0 : 3;
}

int main() {
    struct Case { std::string src; std::string expect; bool present; } cases[] = {
        { "int f(); const int f();\n", "qualifiers differ", true },
        { "int x; const int x;\n", "qualifiers differ", true },
        { "_Atomic(int) a; const _Atomic(int) a;\n", "qualifiers differ", true }
    };

    for (auto &c : cases) {
        int r = run_case(c.src, c.expect, c.present);
        if (r != 0) {
            std::cerr << "case failed for expect='" << c.expect << "' present=" << c.present << "\n";
            return r;
        }
    }

    std::cout << "sema-qualifiers: OK" << std::endl;
    return 0;
}
