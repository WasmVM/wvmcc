#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>

#include "pp/Preprocessor.hpp"

// Drain the pp and return the first string-literal lexeme (empty if none).
static std::string firstStringLexeme(wvmcc::Preprocessor& pp) {
    while (auto t = pp.next()) {
        if (t->kind == wvmcc::PPTokenKind::StringLiteral) return t->lexeme;
    }
    return std::string();
}

int main() {
    const std::string fname = "temp_phase5.c";

    // Valid escapes and UCNs (00C4, 000000A9 are >= 00A0): decoded to UTF-8
    // with no diagnostics.
    {
        std::ofstream ofs(fname);
        ofs << "int \\u00C4x = 0;" << '\n';
        ofs << "const char *s = \"a\\n\\x41\\u00C4\\U000000A9\";" << '\n';
    }
    wvmcc::Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    std::string lex = firstStringLexeme(pp);
    std::string expect = std::string("\"a\nA\xC3\x84\xC2\xA9\"");
    if (lex != expect) {
        std::cerr << "[FAIL] phase5: unexpected decoded lexeme got=" << lex << "\n";
        std::remove(fname.c_str());
        return 3;
    }
    if (!pp.getDiagnostics().empty()) {
        std::cerr << "[FAIL] phase5: unexpected diagnostics on valid UCNs: "
                  << pp.getDiagnostics().front().message << "\n";
        std::remove(fname.c_str());
        return 5;
    }

    // 6.4.3p2: a UCN naming a code point < 00A0 (not $ @ `) in a literal must
    // be diagnosed, not silently decoded.
    {
        std::ofstream ofs(fname);
        ofs << "const char *t = \"\\u0041\";" << '\n';
    }
    wvmcc::Preprocessor pp2;
    if (!pp2.open(fname)) { std::remove(fname.c_str()); return 2; }
    (void)firstStringLexeme(pp2);
    bool found643 = false;
    for (const auto& d : pp2.getDiagnostics())
        if (d.message.find("6.4.3p2") != std::string::npos) found643 = true;
    if (!found643) {
        std::cerr << "[FAIL] phase5: missing 6.4.3p2 diagnostic for literal UCN 0041\n";
        std::remove(fname.c_str());
        return 6;
    }

    std::remove(fname.c_str());
    std::cout << "pp_normalize_test: OK" << std::endl;
    return 0;
}
