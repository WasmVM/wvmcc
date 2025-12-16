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
    // Phase 2: Splice lines ending with a backslash (\\\n)
    static std::string phase2_line_splice(const std::string& input);
    // Phase 3: Replace comments with a single space; retain newlines
    static std::string phase3_strip_comments(const std::string& input);

private:
};

} // namespace wvmcc
