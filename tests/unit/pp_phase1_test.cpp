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
    std::string out = wvmcc::Preprocessor::phase1_normalize(in);
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
            "trigraph basic",
            "??=define LBRACE ??<\n??=define RBRACE ??>\n",
            "#define LBRACE {\n#define RBRACE }\n",
        },
        {
            "no change for lone ?",
            "?x\n",
            "?x\n",
        },
        {
            "CRLF normalization",
            "line1\r\nline2\r\n",
            "line1\nline2\n",
        },
        {
            "CR to LF",
            "line1\rline2\r",
            "line1\nline2\n",
        },
        {
            "ensure final newline",
            "no-final-newline",
            "no-final-newline\n",
        },
    };

    bool all_ok = true;
    for (const auto& c : cases) {
        all_ok &= run_case(c);
    }
    if (!all_ok) return 1;
    std::cout << "pp_phase1_test: all cases passed" << std::endl;
    return 0;
}
