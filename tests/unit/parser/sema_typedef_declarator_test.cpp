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
    const std::string fname = "temp_sema_typedef_decl.c";
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
    // Cases focused on typedef interacting with declarators
    struct Case { std::string src; std::string expect; } cases[] = {
        { "typedef int T; T x; float x;\n", "incompatible declaration" },
        { "typedef int (*F)(int); int g; F g;\n", "incompatible declaration" },
        { "typedef int A[5]; A x; int x[6];\n", "incompatible declaration" },
        { "typedef int ftype(int); ftype g; int g(int, int);\n", "incompatible declaration" },
        // additional targeted cases
        { "typedef int (*PF)(int); int f(int, int); PF f;\n", "incompatible declaration" },
        { "typedef int (*F1)(int,int); typedef int (*F2)(int); F1 a; F2 a;\n", "incompatible declaration" },
        { "typedef int (*F3)(int); int (*a)(int, int); F3 a;\n", "incompatible declaration" },
        { "typedef int T1; typedef T1 T2; T2 x; float x;\n", "incompatible declaration" },
        { "typedef int (*FP)(int); typedef FP FP2; FP2 g; int g(int, int);\n", "incompatible declaration" }
    };

    for (auto &c : cases) {
        int r = run_case(c.src, c.expect);
        if (r != 0) {
            std::cerr << "case failed for expect='" << c.expect << "'\n";
            return r;
        }
    }

    // Additional edge cases: VLA/array-size and function-pointer typedef edges
    struct Case extraCases[] = {
        { "typedef int B[5]; B a; int a[6];\n", "incompatible declaration" },
        { "typedef int ftype(int); int (*p)(int, int); ftype *p;\n", "incompatible declaration" },
        { "typedef int (*PF)(int); typedef PF PF2; PF2 q; int (*q)(int, int);\n", "incompatible declaration" }
    };

    for (auto &c : extraCases) {
        int r = run_case(c.src, c.expect);
        if (r != 0) {
            std::cerr << "extra case failed for expect='" << c.expect << "'\n";
            return r;
        }
    }

        // VLA-specific diagnostics: expect a variably-modified warning at block scope
        struct Case vlaCases[] = {
            { "void h(int n) { int arr[n]; }\n", "variably-modified" },
            { "void f(int n) { typedef int A[n]; A x; }\n", "variably-modified" },
            // VLA at file scope must be an error (C17 §6.9.2 — not allowed)
            { "int n = 5; int arr[n];\n", "variably-modified type not allowed at file scope" },
        };
        for (auto &c : vlaCases) {
            int r = run_case(c.src, c.expect);
            if (r != 0) {
                std::cerr << "vla case failed for expect='" << c.expect << "'\n";
                return r;
            }
        }

    std::cout << "sema-typedef-declarator: OK" << std::endl;
    return 0;
}
