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
    const std::string fname = "temp_sema_thread_local.c";
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
        if (!diags.empty()) {
            std::cerr << "unexpected diagnostics for:\n" << src << "\n";
            for (const auto &d : diags)
                std::cerr << (d.severity==wvmcc::Diagnostic::Severity::Error?"error: ":"note: ") << d.message << "\n";
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
        std::cerr << "--- diagnostics for:\n" << src << "\n---\n";
        for (const auto &d : diags)
            std::cerr << (d.severity==wvmcc::Diagnostic::Severity::Error?"error: ":"note: ") << d.message << "\n";
    }
    std::remove(fname.c_str());
    return found ? 0 : 3;
}

int main() {
    struct Case { std::string src; std::string expect; } cases[] = {
        // _Thread_local with no initializer is valid (C17 §6.7.1)
        { "_Thread_local int x;\n", "" },
        // _Thread_local with constant initializer is valid (C17 §6.7.9 constraint 4)
        { "_Thread_local int x = 42;\n", "" },
        // _Thread_local with non-constant initializer must error
        { "int y = 1; _Thread_local int x = y;\n", "must be constant expression" },
        // _Thread_local combined with extern is valid
        { "extern _Thread_local int x;\n", "" },
        // _Thread_local combined with static is valid
        { "static _Thread_local int x = 0;\n", "" },
    };

    for (auto &c : cases) {
        int r = run_case(c.src, c.expect);
        if (r != 0) {
            std::cerr << "case failed for expect='" << c.expect << "'\n";
            return r;
        }
    }

    std::cout << "sema-thread-local: OK" << std::endl;
    return 0;
}
