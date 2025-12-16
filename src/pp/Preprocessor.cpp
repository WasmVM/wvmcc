#include "Preprocessor.hpp"

#include <fstream>
#include <sstream>

namespace wvmcc {

PreprocessResult Preprocessor::run(const std::string& inputPath) const {
    std::ifstream ifs(inputPath);
    if (!ifs) {
        return PreprocessResult{std::string(), false, std::string("cannot open input: ") + inputPath};
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return PreprocessResult{oss.str(), true, std::string()};
}

} // namespace wvmcc
