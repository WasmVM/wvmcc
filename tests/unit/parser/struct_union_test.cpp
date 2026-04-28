// Unit test: parse struct/union forward declaration and definition with members
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "parser/ASTPrinter.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

int main() {
    const std::string fname = "temp_struct_union_test.c";
    {
        std::ofstream ofs(fname);
        ofs << "struct S;\n";
        ofs << "struct T { int a; unsigned b:3; };\n";
        ofs << "union U { int x; float y; };\n";
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    // Dump tokens for debugging
    {
        Preprocessor pp2;
        if (pp2.open(fname)) {
            Lexer lex2(pp2);
            while (auto t = lex2.next()) {
                std::cerr << "tok: '" << t->lexeme() << "' kind=" << static_cast<int>(t->kind()) << "\n";
            }
        }
    }

    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    if (!tu) { std::remove(fname.c_str()); return 3; }

    // no diagnostics expected for simple well-formed declarations
    if (!parser.getDiagnostics().empty()) {
        for (auto &d : parser.getDiagnostics()) std::cerr << "diag: " << d.message << "\n";
        std::remove(fname.c_str());
        return 4;
    }

    // Print AST and ensure struct/union names and members are present
    std::ostringstream os;
    ASTPrinter p(os);
    p.print(tu);
    std::string out = os.str();
    if (out.find("struct") == std::string::npos) { std::remove(fname.c_str()); return 5; }
    if (out.find("name=\"T\"") == std::string::npos) { std::remove(fname.c_str()); return 6; }
    if (out.find("Bitfield") == std::string::npos) { std::remove(fname.c_str()); return 7; }

    std::cout << "struct-union-test: OK" << std::endl;
    std::remove(fname.c_str());
    return 0;
}
