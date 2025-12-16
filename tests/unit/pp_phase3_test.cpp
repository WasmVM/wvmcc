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
            "block comment replaced by space",
            "int/**/x;\n",
            "int x;\n",
        },
        {
            "block comment spanning lines",
            "a/* hello\nworld */b\n",
            "a b\n",
        },
        {
            "line comment removed, newline kept",
            "x // y\n",
            "x \n",
        },
        {
            "comment markers inside string are not comments",
            "char *s = \"/* not comment */ // nor this\";\n",
            "char *s = \"/* not comment */ // nor this\";\n",
        },
        {
            "comment markers inside char literal are not comments",
            "char c = '/'; // slash\n",
            "char c = '/'; \n",
        },
        {
            "escaped quote in string",
            "const char *s = \"\\\"/*xx*/\\\"\";\n",
            "const char *s = \"\\\"/*xx*/\\\"\";\n",
        },
    };

    bool all_ok = true;
    for (const auto& c : cases) {
        all_ok &= run_case(c);
    }
    if (!all_ok) return 1;
    std::cout << "pp_phase3_test: all cases passed" << std::endl;
    return 0;
}
