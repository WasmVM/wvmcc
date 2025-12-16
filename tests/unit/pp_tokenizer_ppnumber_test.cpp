#include <iostream>
#include <vector>
#include <string>

#include "../../src/pp/Tokenizer.hpp"
#include "../include/test_utils.hpp"

int main() {
    using K = wvmcc::PPTokenKind;
    using testutil::expectKindsLex;
    bool all_ok = true;

    all_ok &= expectKindsLex(
        "integer",
        "123\n",
        {K::PPNumber, K::Newline},
        {"123", "\n"}
    );

    all_ok &= expectKindsLex(
        "float with leading dot",
        ".5\n",
        {K::PPNumber, K::Newline},
        {".5", "\n"}
    );

    all_ok &= expectKindsLex(
        "identifier tail",
        "12abc_DEF\n",
        {K::PPNumber, K::Newline},
        {"12abc_DEF", "\n"}
    );

    all_ok &= expectKindsLex(
        "exponent",
        "1e+10 2E-3\n",
        {K::PPNumber, K::Whitespace, K::PPNumber, K::Newline},
        {"1e+10", " ", "2E-3", "\n"}
    );

    all_ok &= expectKindsLex(
        "hex fp",
        "0x1p-2 0x10P+3\n",
        {K::PPNumber, K::Whitespace, K::PPNumber, K::Newline},
        {"0x1p-2", " ", "0x10P+3", "\n"}
    );

    all_ok &= expectKindsLex(
        "trailing dot",
        "42.\n",
        {K::PPNumber, K::Newline},
        {"42.", "\n"}
    );

    all_ok &= expectKindsLex(
        "leading dot exponent",
        ".3e+2\n",
        {K::PPNumber, K::Newline},
        {".3e+2", "\n"}
    );

    all_ok &= expectKindsLex(
        "hex fp with fractional",
        "0x1.fp0\n",
        {K::PPNumber, K::Newline},
        {"0x1.fp0", "\n"}
    );

    all_ok &= expectKindsLex(
        "suffix-like tail",
        "10ULL 3.14f\n",
        {K::PPNumber, K::Whitespace, K::PPNumber, K::Newline},
        {"10ULL", " ", "3.14f", "\n"}
    );

    all_ok &= expectKindsLex(
        "mixed nondigits",
        "7abc+def\n",
        {K::PPNumber, K::Punctuator, K::Identifier, K::Newline},
        {"7abc", "+", "def", "\n"}
    );

    if (!all_ok) return 1;
    std::cout << "pp_tokenizer_ppnumber_test: all cases passed" << std::endl;
    return 0;
}
