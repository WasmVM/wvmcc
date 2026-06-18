// Regression test for issue #92: the parser used to enter an infinite loop on
// several valid/near-valid constructs (subscripting an array compound literal,
// parenthesized abstract declarators in a type-name, and a comma operator as an
// expression statement). A forward-progress guard now guarantees termination,
// and the comma-operator case parses correctly. Each case below must return
// from parseTranslationUnit(); a reintroduced hang trips the ctest TIMEOUT.
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

// Parse `src` to completion; report the diagnostic count via `outDiagCount`.
static TranslationUnitPtr parse_src(const std::string &src, size_t &outDiagCount) {
    const std::string fname = "temp_no_hang_test.c";
    { std::ofstream ofs(fname); ofs << src; }
    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return nullptr; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();   // must terminate (no hang)
    outDiagCount = parser.getDiagnostics().size();
    std::remove(fname.c_str());
    return tu;
}

int main() {
    // Former hangs: must simply terminate. They are not yet fully supported, so
    // we only require that parsing returns (the value of the diagnostics is not
    // asserted here — the point is "no infinite loop").
    const char *terminates[] = {
        "int main(void) { return (int[]){10, 20, 30}[1]; }\n",        // compound-literal subscript
        "int main(void) { return sizeof(int(*)(void)); }\n",          // ptr-to-function type-name
        "int main(void) { return sizeof(int(*)[3]); }\n",             // ptr-to-array type-name
        "int main(void) { return _Generic((int*)0, int*: 4, default: 5); }\n", // abstract ptr assoc
    };
    for (const char *src : terminates) {
        size_t n = 0;
        if (!parse_src(src, n)) { std::cerr << "parse returned null for: " << src; return 1; }
    }

    // Comma operator as an expression statement now parses correctly: the
    // statement `n = 0, n += 3;` is a single comma expression, no diagnostics.
    {
        size_t n = 0;
        auto tu = parse_src("void f(void) { int n = 0; n = 0, n += 3; }\n", n);
        if (!tu) { std::cerr << "comma-statement parse returned null\n"; return 2; }
        if (n != 0) { std::cerr << "comma-statement produced " << n << " diagnostic(s), expected 0\n"; return 3; }
        // Locate the function body and the comma expression statement.
        if (tu->externals.size() != 1) { std::cerr << "expected 1 external decl\n"; return 4; }
        auto f = std::get<FunctionDefPtr>(tu->externals[0]->decl);
        if (!f || f->body.size() < 2) { std::cerr << "expected >=2 block items\n"; return 5; }
        auto st = std::get<StmtPtr>(f->body[1]->item);
        auto es = std::dynamic_pointer_cast<ExprStmt>(st);
        if (!es) { std::cerr << "second block item is not an expression statement\n"; return 6; }
        auto be = std::dynamic_pointer_cast<BinaryExpr>(es->expr);
        if (!be || be->op != ",") { std::cerr << "expression statement is not a comma expression\n"; return 7; }
    }

    std::cout << "parser-no-hang: OK" << std::endl;
    return 0;
}
