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
        // file-scope static asserts
        { "_Static_assert(1, \"ok\");\n", "" },
        { "_Static_assert(0, \"failed message\");\n", "failed message" },
        { "int x = 1; _Static_assert(x, \"non-const\");\n", "_Static_assert requires an integer constant expression" },
        // block-scope static asserts (C17 §6.8.2)
        { "void f(void) { _Static_assert(1, \"ok\"); }\n", "" },
        { "void f(void) { _Static_assert(0, \"block failed\"); }\n", "block failed" },
        { "void f(void) { int x = 1; _Static_assert(2 > 1, \"math broken\"); }\n", "" },
        { "void f(void) { _Static_assert(1 == 1, \"identity failed\"); }\n", "" },
        // issue #81: sizeof / _Alignof are integer constant expressions (LP64).
        { "_Static_assert(sizeof(int) == 4, \"sizeof int\");\n", "" },
        { "_Static_assert(sizeof(long) == 8, \"sizeof long\");\n", "" },
        { "_Static_assert(sizeof(char) == 1, \"sizeof char\");\n", "" },
        { "_Static_assert(_Alignof(int) == 4, \"alignof int\");\n", "" },
        { "_Static_assert(_Alignof(long) == 8, \"alignof long\");\n", "" },
        { "_Static_assert(sizeof(int) == 8, \"int is not 8\");\n", "int is not 8" },
        // aggregate sizeof / _Alignof
        { "struct S { int a; int b; }; _Static_assert(sizeof(struct S) == 8, \"agg size\");\n", "" },
        { "struct P { char c; int i; }; _Static_assert(sizeof(struct P) == 8, \"padding\");\n", "" },
        { "union U { char c; long l; }; _Static_assert(_Alignof(union U) == 8, \"union align\");\n", "" },
        // issue #81: casts of arithmetic constants are integer constant expressions.
        { "_Static_assert((int)1 == 1, \"cast int\");\n", "" },
        { "_Static_assert((_Bool)5 == 1, \"cast bool\");\n", "" },
        { "_Static_assert((unsigned char)-1 == 255, \"cast uchar\");\n", "" },
        { "_Static_assert((short)-1 < 0, \"cast short keeps sign\");\n", "" },
        // pointer cast is not an arithmetic constant: stays a non-ICE.
        { "_Static_assert((int *)0 == 0, \"ptr cast\");\n", "_Static_assert requires an integer constant expression" },
        // deferred from issue #82: unsigned comparison semantics in the
        // semantic ICE evaluator (UINT64_MAX must compare greater than 0).
        { "_Static_assert(18446744073709551615ULL > 0, \"u max\");\n", "" },
        { "_Static_assert((unsigned)-1 > 0, \"cast unsigned\");\n", "" },
        { "_Static_assert(-1 < 0, \"signed still works\");\n", "" },
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
