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
        // issue #81 residuals: cast operands under multiplicative operators.
        { "_Static_assert((int)6 / (int)2 == 3, \"mult cast div\");\n", "" },
        { "_Static_assert((int)7 % (int)3 == 1, \"mult cast mod\");\n", "" },
        // issue #81 residual: sizeof of an *expression* (object), resolved via
        // the semantic type resolver.
        { "int v81; _Static_assert(sizeof(v81) == 4, \"sizeof expr\");\n", "" },
        // issue #81 residual: _Generic selection in a constant expression.
        { "_Static_assert(_Generic(0, int: 1, default: 0) == 1, \"generic int\");\n", "" },
        // issue #81: a literal's resolved rank drives _Generic selection, so
        // `0L` is long and `0LL` is long long (not int).
        { "_Static_assert(_Generic(0L, long: 1, int: 2, default: 0) == 1, \"generic long\");\n", "" },
        { "_Static_assert(_Generic(0LL, long long: 1, int: 2, default: 0) == 1, \"generic long long\");\n", "" },
        { "_Static_assert(_Generic(0U, unsigned: 1, int: 2, default: 0) == 1, \"generic unsigned\");\n", "" },
        { "_Static_assert(_Generic(0UL, unsigned long: 1, default: 0) == 1, \"generic ulong\");\n", "" },
        // #92-followup: short-circuit operators — the unevaluated operand need
        // not be a constant (6.6p3), so `2 || 1/0` is a valid ICE worth 1.
        { "_Static_assert((2 || 1 / 0) == 1, \"|| short-circuits\");\n", "" },
        { "_Static_assert((0 && 1 / 0) == 0, \"&& short-circuits\");\n", "" },
        { "_Static_assert((1 ? 5 : 1 / 0) == 5, \"?: chooses one arm\");\n", "" },
        // A non-short-circuited division by zero is still not a constant.
        { "_Static_assert((1 / 0) == 0, \"div-by-zero rejected\");\n", "_Static_assert requires an integer constant expression" },
        // Floating-constant relational folding (wvmcc relaxation of 6.6p6).
        { "_Static_assert(1.0 + 2.22e-16 > 1.0, \"float arithmetic + compare\");\n", "" },
        { "_Static_assert(0.5F > 0, \"float vs int compare\");\n", "" },
        { "_Static_assert(1.0 > 2.0, \"false float compare fails\");\n", "static assertion failed" },
        // Enumeration constant inside a block-scope constant expression resolves
        // (folding is allowed in a required constant-expression context).
        { "enum { EA = 13 }; void f(void) { _Static_assert(EA == 13, \"enum in block ICE\"); }\n", "" },
        // sizeof a struct/union typedef resolves inside a constant expression.
        { "typedef struct { long a; long b; } Pair; _Static_assert(sizeof(Pair) == 16, \"struct typedef size\");\n", "" },
        // static_assert-declaration as a struct/union member (C17 6.7.2.1).
        // These previously sent the struct-member parser into an infinite loop
        // (the keyword was neither a specifier nor a declarator, so the loop
        // never advanced); ensure they parse and evaluate now.
        { "struct S { int m; _Static_assert(1, \"member ok\"); };\n", "" },
        { "struct S { int m; _Static_assert(sizeof(int) == 4, \"member sizeof\"); };\n", "" },
        { "struct S { int m; _Static_assert(1 == 2, \"member boom\"); };\n", "member boom" },
        { "union U { int m; _Static_assert(1, \"union member ok\"); };\n", "" },
        { "struct S { _Static_assert(1, \"only assert\"); };\n", "" },
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
