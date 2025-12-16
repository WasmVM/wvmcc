#include <iostream>
#include <vector>
#include <string>

#include "../../src/pp/Tokenizer.hpp"

static bool expectKinds(const std::string& name, const std::string& input,
                        const std::vector<wvmcc::PPTokenKind>& kinds,
                        const std::vector<std::string>& lexemes) {
    auto toks = wvmcc::Tokenizer::tokenize_minimal(input);
    bool ok = toks.size() == kinds.size();
    if (!ok) {
        std::cerr << "[FAIL] " << name << ": size mismatch: expected "
                  << kinds.size() << ", got " << toks.size() << std::endl;
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

    all_ok &= expectKinds(
        "spaces and letters",
        "  abc\n",
        {K::Whitespace, K::Other, K::Other, K::Other, K::Newline},
        {"  ", "a", "b", "c", "\n"}
    );

    all_ok &= expectKinds(
        "tabs and punctuation",
        "\t= +\n",
        {K::Whitespace, K::Other, K::Whitespace, K::Other, K::Newline},
        {"\t", "=", " ", "+", "\n"}
    );

    all_ok &= expectKinds(
        "mixed vtab/ff",
        "\v\fX\n",
        {K::Whitespace, K::Other, K::Newline},
        {"\v\f", "X", "\n"}
    );

    if (!all_ok) return 1;
    std::cout << "pp_tokenizer_min_test: all cases passed" << std::endl;
    return 0;
}
