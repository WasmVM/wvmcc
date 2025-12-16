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
    std::string out = wvmcc::Preprocessor::phase2_line_splice(in);
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
            "basic splice",
            "abc\\\n123\n",
            "abc123\n",
        },
        {
            "chain splice across lines",
            "a\\\n b\\\n c\n",
            "a b c\n",
        },
        {
            "no splice when backslash not last",
            "x\\ y\n",
            "x\\ y\n",
        },
        {
            "multiple backslashes only last eligible",
            "p\\\\\\\nq\n",
            "p\\\\q\n",
        },
        {
            "empty line with backslash splices",
            "\\\nnext\n",
            "next\n",
        },
    };

    bool all_ok = true;
    for (const auto& c : cases) {
        all_ok &= run_case(c);
    }
    if (!all_ok) return 1;
    std::cout << "pp_phase2_test: all cases passed" << std::endl;
    return 0;
}
