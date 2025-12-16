#include <iostream>
#include <string>
#include <vector>

#include "../../src/pp/Preprocessor.hpp"

struct Case {
    const char* name;
    const char* input;
    const char* expected;
};

static bool run_case(const Case& c) {
    std::string in(c.input);
    std::string out = wvmcc::Preprocessor::phase1to3_process(in);
    bool ok = (out == std::string(c.expected));
    if (!ok) {
        std::cerr << "[FAIL] " << c.name << "\nExpected:\n" << c.expected
                  << "\nGot:\n" << out << std::endl;
    }
    return ok;
}

int main() {
    std::vector<Case> cases = {
        {
            "trigraph then comment",
            "??=define X 1 /*comment*/\n",
            "#define X 1 \n",
        },
        {
            "splicing across comment start (not inside string)",
            "int a = 1\\\n/* remove */int b=2;\n",
            "int a = 1 int b=2;\n",
        },
        {
            "line comment after splice",
            "abc\\\n// hidden\nxyz\n",
            "abc\nxyz\n",
        },
        {
            "backslash in string not spliced",
            "const char *s=\"line\\\\\n\"; // comment\n",
            "const char *s=\"line\\\\\n\"; \n",
        },
        {
            "comment markers in string and char",
            "char *s=\"// not comment /* nor */\"; char c='*';\n",
            "char *s=\"// not comment /* nor */\"; char c='*';\n",
        },
        {
            "nested comment start tokens (no nesting supported)",
            "a/* one /* two */ b */ c\n",
            "a  b */ c\n",
        },
        {
            "CRLF with splice and block comment",
            "x\\\r\n/*y*/\r\nz\r\n",
            "x \nz\n",
        },
        {
            "ensure final newline added",
            "int x;",
            "int x;\n",
        },
    };

    bool all_ok = true;
    for (const auto& c : cases) {
        all_ok &= run_case(c);
    }
    if (!all_ok) return 1;
    std::cout << "pp_phase_combined_test: all cases passed" << std::endl;
    return 0;
}
