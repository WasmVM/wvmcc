#include <iostream>
#include <vector>
#include <string>

#include "pp/Tokenizer.hpp"
#include "../include/test_utils.hpp"

int main() {
    using K = wvmcc::PPTokenKind;
    bool all_ok = true;

    using testutil::expectKindsLex;
    all_ok &= expectKindsLex(
        "simple puncts",
        "[](){}.,;\n",
        {K::Punctuator, K::Punctuator, K::Punctuator, K::Punctuator,K::Punctuator, K::Punctuator, K::Punctuator, K::Punctuator,K::Punctuator, K::Newline},
        {"[", "]", "(", ")", "{", "}", ".", ",", ";", "\n"}
    );

    all_ok &= expectKindsLex(
        "multi-char",
        "-> ++ -- << >> <= >= == != && || ... *= /= %= += -= <<= >>= &= ^= |= ##\n",
        {
            K::Punctuator, K::Whitespace, // ->
            K::Punctuator, K::Whitespace, // ++
            K::Punctuator, K::Whitespace, // --
            K::Punctuator, K::Whitespace, // <<
            K::Punctuator, K::Whitespace, // >>
            K::Punctuator, K::Whitespace, // <=
            K::Punctuator, K::Whitespace, // >=
            K::Punctuator, K::Whitespace, // ==
            K::Punctuator, K::Whitespace, // !=
            K::Punctuator, K::Whitespace, // &&
            K::Punctuator, K::Whitespace, // ||
            K::Punctuator, K::Whitespace, // ...
            K::Punctuator, K::Whitespace, // *=
            K::Punctuator, K::Whitespace, // /=
            K::Punctuator, K::Whitespace, // %=
            K::Punctuator, K::Whitespace, // +=
            K::Punctuator, K::Whitespace, // -=
            K::Punctuator, K::Whitespace, // <<=
            K::Punctuator, K::Whitespace, // >>=
            K::Punctuator, K::Whitespace, // &=
            K::Punctuator, K::Whitespace, // ^=
            K::Punctuator, K::Whitespace, // |=
            K::Punctuator, K::Newline     // ##
        },
        {"->", " ",
         "++", " ",
         "--", " ",
         "<<", " ",
         ">>", " ",
         "<=", " ",
         ">=", " ",
         "==", " ",
         "!=", " ",
         "&&", " ",
         "||", " ",
         "...", " ",
         "*=", " ",
         "/=", " ",
         "%=", " ",
         "+=", " ",
         "-=", " ",
         "<<=", " ",
         ">>=", " ",
         "&=", " ",
         "^=", " ",
         "|=", " ",
         "##", "\n"}
    );

    all_ok &= expectKindsLex(
        "digraphs",
        "<: :> <% %> %: %:%:\n",
        {K::Punctuator, K::Whitespace, K::Punctuator, K::Whitespace, K::Punctuator, K::Whitespace, K::Punctuator, K::Whitespace, K::Punctuator, K::Whitespace, K::Punctuator, K::Newline},
        {"<:", " ", ":>", " ", "<%", " ", "%>", " ", "%:", " ", "%:%:", "\n"}
    );

    if (!all_ok) return 1;
    std::cout << "pp_tokenizer_punct_test: all cases passed" << std::endl;
    return 0;
}
