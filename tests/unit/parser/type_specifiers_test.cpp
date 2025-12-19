// Unit test: ensure type-specifier sequences are parsed and printed
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
    const std::string fname = "temp_type_specifiers_test.c";
    {
        std::ofstream ofs(fname);
        ofs << "unsigned long int a;\n";
        ofs << "long double b;\n";
        ofs << "const volatile int c;\n";
        ofs << "inline int f() { return 0; }\n";
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    if (!tu) { std::remove(fname.c_str()); return 3; }

    // no diagnostics expected
    if (!parser.getDiagnostics().empty()) {
        for (auto &d : parser.getDiagnostics()) std::cerr << "diag: " << d.message << "\n";
        std::remove(fname.c_str());
        return 4;
    }

    std::ostringstream os;
    ASTPrinter p(os);
    p.print(tu);
    std::string out = os.str();

    // Check for presence of expected specifier text
    if (out.find("unsigned") == std::string::npos) { std::remove(fname.c_str()); return 5; }
    if (out.find("long") == std::string::npos) { std::remove(fname.c_str()); return 6; }
    if (out.find("long") == std::string::npos || out.find("double") == std::string::npos) { std::remove(fname.c_str()); return 7; }
    if (out.find("const") == std::string::npos || out.find("volatile") == std::string::npos) { std::remove(fname.c_str()); return 8; }
    if (out.find("inline") == std::string::npos) { std::remove(fname.c_str()); return 9; }

    std::cout << "type-specifiers-test: OK" << std::endl;
    std::remove(fname.c_str());
    return 0;
}
