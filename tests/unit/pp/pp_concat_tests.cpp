#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <vector>

#include "pp/Preprocessor.hpp"

static std::vector<std::string> collect_string_literals(const std::string &fname) {
    std::vector<std::string> lits;
    wvmcc::Preprocessor pp;
    if (!pp.open(fname)) return lits;
    while (auto t = pp.next()) {
        if (t->kind == wvmcc::PPTokenKind::StringLiteral) lits.push_back(t->lexeme);
    }
    return lits;
}

static int run_simple_concat_test() {
    const std::string fname = "temp_phase6_simple.c";
    {
        std::ofstream ofs(fname);
        ofs << "const char *s1 = \"hello \" \"world\";\n";
        ofs << "const char *s2 = u8\"ab\" \"cd\";\n";
        ofs << "const char *s3 = u8\"one\" L\"two\";\n";
    }
    auto lits = collect_string_literals(fname);
    std::remove(fname.c_str());
    if (lits.size() < 4) { std::cerr << "[FAIL] simple: got "<<lits.size()<<" literals\n"; return 1; }
    if (lits[0] != "\"hello world\"") { std::cerr<<"[FAIL] simple: expected \"hello world\", got '"<<lits[0]<<"'\n"; return 2; }
    if (lits[1] != "u8\"abcd\"") { std::cerr<<"[FAIL] simple: expected u8\"abcd\", got '"<<lits[1]<<"'\n"; return 3; }
    if (!(lits[2]=="u8\"one\"" && lits[3]=="L\"two\"")) { std::cerr<<"[FAIL] simple: incompatible mix expected u8\"one\", L\"two\"; got '"<<lits[2]<<"','"<<lits[3]<<"'\n"; return 4; }
    return 0;
}

static int run_macro_concat_test() {
    const std::string fname = "temp_phase6_macro.c";
    {
        std::ofstream ofs(fname);
        ofs << "#define A \"foo\"\n";
        ofs << "#define B u8\"ab\"\n";
        ofs << "const char *s1 = A \"bar\";\n";
        ofs << "const char *s2 = B \"cd\";\n";
    }
    auto lits = collect_string_literals(fname);
    std::remove(fname.c_str());
    if (lits.size() < 2) { std::cerr << "[FAIL] macro: got "<<lits.size()<<" literals\n"; return 1; }
    if (lits[0] != "\"foobar\"") { std::cerr<<"[FAIL] macro: expected \"foobar\", got '"<<lits[0]<<"'\n"; return 2; }
    if (lits[1] != "u8\"abcd\"") { std::cerr<<"[FAIL] macro: expected u8\"abcd\", got '"<<lits[1]<<"'\n"; return 3; }
    return 0;
}

static int run_multi_concat_test() {
    const std::string fname = "temp_phase6_multi.c";
    {
        std::ofstream ofs(fname);
        ofs << "const char *s1 = \"a\" \"b\" \"c\";\n";
        ofs << "const char *s2 = u8\"x\" \"y\" \"z\";\n";
        // s3 mixes a UTF-8 and a wide prefix in one adjacent run, which is a
        // constraint violation (C17 6.4.5p5: such a sequence "shall not include
        // both a wide string literal and a UTF-8 string literal"). A diagnostic
        // is required; the grouping of the leftover tokens after the diagnostic
        // is unspecified. Concatenation proceeds pairwise left-to-right, so the
        // u8/L pair stops at u8"one" and the still-compatible L"two" "three"
        // (wide + unprefixed) then joins to L"twothree".
        ofs << "const char *s3 = u8\"one\" L\"two\" \"three\";\n";
    }
    auto lits = collect_string_literals(fname);
    std::remove(fname.c_str());
    if (lits.size() < 4) { std::cerr<<"[FAIL] multi: got "<<lits.size()<<" literals\n"; return 1; }
    if (lits[0] != "\"abc\"") { std::cerr<<"[FAIL] multi: s1 expected \"abc\", got '"<<lits[0]<<"'\n"; return 2; }
    if (lits[1] != "u8\"xyz\"") { std::cerr<<"[FAIL] multi: s2 expected u8\"xyz\", got '"<<lits[1]<<"'\n"; return 3; }
    if (!(lits[2]=="u8\"one\"" && lits[3]=="L\"twothree\"")) { std::cerr<<"[FAIL] multi: s3 mismatch, got '"<<lits[2]<<"','"<<(lits.size()>3?lits[3]:std::string("<none>"))<<"'\n"; return 4; }
    return 0;
}

int main() {
    if (int r = run_simple_concat_test()) return r;
    if (int r = run_macro_concat_test()) return r;
    if (int r = run_multi_concat_test()) return r;
    std::cout << "pp_concat_tests: OK" << std::endl;
    return 0;
}
