#include <iostream>
#include <vector>
#include <string>

#include "../../src/pp/Tokenizer.hpp"
#include "../include/test_utils.hpp"

int main() {
    using K = wvmcc::PPTokenKind;
    bool all_ok = true;

    using testutil::expectKindsLex;
    all_ok &= expectKindsLex(
        "plain string",
        "\"xyz\"\n",
        {K::StringLiteral, K::Newline},
        {"\"xyz\"", "\n"}
    );

    all_ok &= expectKindsLex(
        "prefixed strings",
        "u8\"a\" u\"b\" U\"c\" L\"d\"\n",
        {K::StringLiteral, K::Whitespace, K::StringLiteral, K::Whitespace, K::StringLiteral, K::Whitespace, K::StringLiteral, K::Newline},
        {"u8\"a\"", " ", "u\"b\"", " ", "U\"c\"", " ", "L\"d\"", "\n"}
    );

    all_ok &= expectKindsLex(
        "escaped quote",
        "\"a\\\"b\"\n",
        {K::StringLiteral, K::Newline},
        {"\"a\\\"b\"", "\n"}
    );

    all_ok &= expectKindsLex(
        "adjacent strings",
        "\"a\" \"b\"\n",
        {K::StringLiteral, K::Whitespace, K::StringLiteral, K::Newline},
        {"\"a\"", " ", "\"b\"", "\n"}
    );

    all_ok &= expectKindsLex(
        "unterminated string",
        "\"abc\n",
        {K::StringLiteral, K::Newline},
        {"\"abc", "\n"}
    );

    if (!all_ok) return 1;
    std::cout << "pp_tokenizer_string_test: all cases passed" << std::endl;
    return 0;
}
