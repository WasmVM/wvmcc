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
        "plain char",
        "'a'\n",
        {K::CharConst, K::Newline},
        {"'a'", "\n"}
    );

    all_ok &= expectKindsLex(
        "prefixed chars",
        "L'a' u'\\n' U'\\t'\n",
        {K::CharConst, K::Whitespace, K::CharConst, K::Whitespace, K::CharConst, K::Newline},
        {"L'a'", " ", "u'\\n'", " ", "U'\\t'", "\n"}
    );

    all_ok &= expectKindsLex(
        "escaped quote",
        "'\\''\n",
        {K::CharConst, K::Newline},
        {"'\\''", "\n"}
    );

    all_ok &= expectKindsLex(
        "hex/octal/universal",
        "'\\x41' '\\101' '\\u00A9' '\\U0001F600'\n",
        {K::CharConst, K::Whitespace, K::CharConst, K::Whitespace, K::CharConst, K::Whitespace, K::CharConst, K::Newline},
        {"'\\x41'", " ", "'\\101'", " ", "'\\u00A9'", " ", "'\\U0001F600'", "\n"}
    );

    all_ok &= expectKindsLex(
        "unterminated char",
        "'abc\n",
        {K::CharConst, K::Newline},
        {"'abc", "\n"}
    );

    if (!all_ok) return 1;
    std::cout << "pp_tokenizer_char_test: all cases passed" << std::endl;
    return 0;
}
