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
};

} // namespace wvmcc
