#pragma once

#include <string>
#include <vector>
#include "Tokenizer.hpp"

namespace wvmcc {

struct PreprocessResult {
    std::vector<PPToken> tokens;
    bool success{true};
    std::string errorMsg;
};

class Preprocessor {
public:
    PreprocessResult run(const std::string& inputPath) const;
    // Combined Phase 1–3 single-pass processing for performance
    static std::string phase1to3_process(const std::string& input);

private:
};

} // namespace wvmcc
