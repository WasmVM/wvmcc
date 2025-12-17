#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "pp/Tokenizer.hpp"

struct Case {
    const char* name;
    const char* input;
    const char* expected;
};

static bool run_case(const Case& c) {
    std::istringstream iss(c.input);
    // Single-pass tokenizer: concatenate lexemes to approximate preprocessed stream
    wvmcc::Tokenizer tokenizer(iss);
    std::vector<wvmcc::PPToken> toks;
    for (const auto& t : tokenizer) {
        toks.push_back(t);
    }
    std::string out;
    out.reserve(256);
    for (auto& t : toks) out += t.lexeme;
    bool ok = (out == std::string(c.expected));
    if (!ok) {
        std::cerr << "[FAIL] " << c.name << "\nExpected:\n" << c.expected
                  << "\nGot:\n" << out << std::endl;
    }
    return ok;
}

int main() {
    std::vector<Case> cases = {
        {"trigraph basic", "??=define LBRACE ??<\n??=define RBRACE ??>\n", "#define LBRACE {\n#define RBRACE }\n"},
        {"no change for lone ?", "?x\n", "?x\n"},
        {"CRLF normalization", "line1\r\nline2\r\n", "line1\nline2\n"},
        {"CR to LF", "line1\rline2\r", "line1\nline2\n"},
        {"basic splice", "abc\\\n123\n", "abc123\n"},
        {"chain splice across lines", "a\\\n b\\\n c\n", "a b c\n"},
        {"no splice when backslash not last", "x\\ y\n", "x\\ y\n"},
        {"multiple backslashes only last eligible", "p\\\\\\\nq\n", "p\\\\q\n"},
        {"empty line with backslash splices", "\\\nnext\n", "next\n"},
        {"block comment replaced by space", "int/**/x;\n", "int x;\n"},
        {"block comment spanning lines", "a/* hello\nworld */b\n", "a b\n"},
        {"line comment removed, newline kept", "x // y\n", "x \n"},
        {"comment markers inside string are not comments", "char *s = \"/* not comment */ // nor this\";\n", "char *s = \"/* not comment */ // nor this\";\n"},
        {"comment markers inside char literal are not comments", "char c = '/'; // slash\n", "char c = '/'; \n"},
        {"escaped quote in string", "const char *s = \"\\\"/*xx*/\\\"\";\n", "const char *s = \"\\\"/*xx*/\\\"\";\n"},
        {"trigraph then comment", "??=define X 1 /*comment*/\n", "#define X 1 \n"},
        {"splicing across comment start (not inside string)", "int a = 1\\\n/* remove */int b=2;\n", "int a = 1 int b=2;\n"},
        {"line comment after splice", "abc\\\n// hidden\nxyz\n", "abc\nxyz\n"},
        {"backslash in string not spliced", "const char *s=\"line\\\\\n\"; // comment\n", "const char *s=\"line\\\\\n\"; \n"},
        {"comment markers in string and char", "char *s=\"// not comment /* nor */\"; char c='*';\n", "char *s=\"// not comment /* nor */\"; char c='*';\n"},
        {"nested comment start tokens (no nesting supported)", "a/* one /* two */ b */ c\n", "a  b */ c\n"},
        {"CRLF with splice and block comment", "x\\\r\n/*y*/\r\nz\r\n", "x \nz\n"},
    };

    bool all_ok = true;
    for (const auto& c : cases) {
        all_ok &= run_case(c);
    }
    if (!all_ok) return 1;
    std::cout << "pp_basic_test: all cases passed" << std::endl;
    return 0;
}
