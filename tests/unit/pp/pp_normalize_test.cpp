#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>

#include "pp/Preprocessor.hpp"

int main() {
    const std::string fname = "temp_phase5.c";
    {
        std::ofstream ofs(fname);
        ofs << "const char *s = \"a\\n\\x41\\u0042\\U00000041\";\n";
    }
    wvmcc::Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    bool found = false;
    while (auto t = pp.next()) {
        if (t->kind == wvmcc::PPTokenKind::StringLiteral) {
            // Expect inner content decoded: a newline + 'A' + 'B' + 'A' => "a\nABA"
            std::string expect = "\"a\nABA\"";
            if (t->lexeme != expect) {
                std::cerr << "[FAIL] phase5: expected lexeme='" << expect << "' got='" << t->lexeme << "'\n";
                std::remove(fname.c_str());
                return 3;
            }
            found = true; break;
        }
    }
    std::remove(fname.c_str());
    if (!found) return 4;
    std::cout << "pp_normalize_test: OK" << std::endl;
    return 0;
}
