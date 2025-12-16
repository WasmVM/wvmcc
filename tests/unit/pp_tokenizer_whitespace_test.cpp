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
        "spaces and letters",
        "  abc\n",
        {K::Whitespace, K::Other, K::Other, K::Other, K::Newline},
        {"  ", "a", "b", "c", "\n"}
    );

    all_ok &= expectKindsLex(
        "tabs and punctuation",
        "\t= +\n",
        {K::Whitespace, K::Punctuator, K::Whitespace, K::Punctuator, K::Newline},
        {"\t", "=", " ", "+", "\n"}
    );

    all_ok &= expectKindsLex(
        "mixed vtab/ff",
        "\v\fX\n",
        {K::Whitespace, K::Other, K::Newline},
        {"\v\f", "X", "\n"}
    );

    if (!all_ok) return 1;
    std::cout << "pp_tokenizer_whitespace_test: all cases passed" << std::endl;
    return 0;
}
