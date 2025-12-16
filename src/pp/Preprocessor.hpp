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

private:
};

} // namespace wvmcc
