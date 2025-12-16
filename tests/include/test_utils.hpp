#pragma once

#include <iostream>
#include <string>
#include <vector>

// Requires the including file to include "../../src/pp/Tokenizer.hpp" first.

namespace testutil {

inline bool expectKindsLex(const std::string& name,
                           const std::string& input,
                           const std::vector<wvmcc::PPTokenKind>& kinds,
                           const std::vector<std::string>& lexemes) {
    wvmcc::Tokenizer tokenizer(input);
    auto toks = tokenizer.tokenize();
    if (toks.size() != kinds.size() || toks.size() != lexemes.size()) {
        std::cerr << "[FAIL] " << name << ": size mismatch: expected kinds="
                  << kinds.size() << ", lexemes=" << lexemes.size()
                  << ", got tokens=" << toks.size() << std::endl;
        std::cerr << "  tokens:" << std::endl;
        for (size_t i = 0; i < toks.size(); ++i) {
            std::cerr << "    [" << i << "] kind=" << (int)toks[i].kind
                      << " lexeme='" << toks[i].lexeme << "'" << std::endl;
        }
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

} // namespace testutil
