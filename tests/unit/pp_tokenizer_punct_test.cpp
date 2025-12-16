#include <iostream>
#include <vector>
#include <string>

#include "../../src/pp/Tokenizer.hpp"

static bool expectKindsLex(const std::string& name, const std::string& input,
                           const std::vector<wvmcc::PPTokenKind>& kinds,
                           const std::vector<std::string>& lexemes) {
    auto toks = wvmcc::Tokenizer::tokenize_with_punctuators(input);
    if (toks.size() != kinds.size() || toks.size() != lexemes.size()) {
        std::cerr << "[FAIL] " << name << ": size mismatch: expected kinds="
                  << kinds.size() << ", lexemes=" << lexemes.size()
                  << ", got tokens=" << toks.size() << std::endl;
        return false;
    }
    for (size_t i = 0; i < kinds.size(); ++i) {
        if (toks[i].kind != kinds[i] || toks[i].lexeme != lexemes[i]) {
            std::cerr << "[FAIL] " << name << ": token " << i
                      << " kind/lexeme mismatch\n  expected kind=" << (int)kinds[i]
                      << " lexeme='" << lexemes[i] << "'\n  got kind="
                      << (int)toks[i].kind << " lexeme='" << toks[i].lexeme << "'\n";
            return false;
        }
    }
    return true;
}

int main() {
    using K = wvmcc::PPTokenKind;
    bool all_ok = true;

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
