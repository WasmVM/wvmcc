#pragma once

#include <string>

namespace wvmcc {

struct PreprocessResult {
    std::string text;
    bool success{true};
    std::string errorMsg;
};

class Preprocessor {
public:
    PreprocessResult run(const std::string& inputPath) const;
    // Phase 1: Source mapping + trigraphs + EOL normalization + final newline
    static std::string phase1_normalize(const std::string& input);

private:
};

} // namespace wvmcc
