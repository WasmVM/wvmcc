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
