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
    const std::string fname = "temp_sema_decl_compat.c";
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
    // Cases: incompatible object redeclaration, incompatible function return type,
    // incompatible function parameter count
    struct Case { std::string src; std::string expect; } cases[] = {
        { "int x; float x;\n", "incompatible declaration" },
        { "int f(int a); float f(int a);\n", "incompatible declaration" },
        { "int g(int); int g(int, int);\n", "incompatible declaration" }
    };

    for (auto &c : cases) {
        int r = run_case(c.src, c.expect);
        if (r != 0) {
            std::cerr << "case failed for expect='" << c.expect << "'\n";
            return r;
        }
    }

    std::cout << "sema-decl-compat: OK" << std::endl;
    return 0;
}
