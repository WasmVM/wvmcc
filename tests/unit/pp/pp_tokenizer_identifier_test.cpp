#include <iostream>
#include <vector>
#include <string>

#include "pp/Tokenizer.hpp"
#include "../include/test_utils.hpp"

int main() {
    using K = wvmcc::PPTokenKind;
    using testutil::expectKindsLex;
    bool all_ok = true;

    all_ok &= expectKindsLex(
        "basic identifiers",
        "foo _bar baz2\n",
        {K::Identifier, K::Whitespace, K::Identifier, K::Whitespace, K::Identifier, K::Newline},
        {"foo", " ", "_bar", " ", "baz2", "\n"}
    );

    all_ok &= expectKindsLex(
        "identifier with UCN \\\u0041bc",
        "\\u0041bc\n",
        {K::Identifier, K::Newline},
        {"\\u0041bc", "\n"}
    );

    all_ok &= expectKindsLex(
        "identifier with UCN underscore",
        "x\\U0000005Fyz\n",
        {K::Identifier, K::Newline},
        {"x\\U0000005Fyz", "\n"}
    );

    all_ok &= expectKindsLex(
        "mixed identifier",
        "_x1\\u00AA3\n",
        {K::Identifier, K::Newline},
        {"_x1\\u00AA3", "\n"}
    );

    if (!all_ok) return 1;
    std::cout << "pp_tokenizer_identifier_test: all cases passed" << std::endl;
    return 0;
}
