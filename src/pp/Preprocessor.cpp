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
    std::string normalized = phase1_normalize(oss.str());
    return PreprocessResult{normalized, true, std::string()};
}

// Phase 1 implementation: trigraphs, EOL normalization, final newline
std::string Preprocessor::phase1_normalize(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    // Replace trigraphs
    for (size_t i = 0; i < input.size(); ++i) {
        if (i + 2 < input.size() && input[i] == '?' && input[i + 1] == '?') {
            char third = input[i + 2];
            switch (third) {
                case '=': out.push_back('#'); i += 2; continue;
                case '(': out.push_back('['); i += 2; continue;
                case '/': out.push_back('\\'); i += 2; continue;
                case ')': out.push_back(']'); i += 2; continue;
                case '\'': out.push_back('^'); i += 2; continue;
                case '<': out.push_back('{'); i += 2; continue;
                case '!': out.push_back('|'); i += 2; continue;
                case '>': out.push_back('}'); i += 2; continue;
                case '-': out.push_back('~'); i += 2; continue;
                default: break; // not a trigraph, fall through
            }
        }
        out.push_back(input[i]);
    }

    // Normalize end-of-line: treat CRLF and CR as LF ('\n')
    std::string eol;
    eol.reserve(out.size());
    for (size_t i = 0; i < out.size(); ++i) {
        char c = out[i];
        if (c == '\r') {
            // If CRLF, skip CR and ensure single LF
            if (i + 1 < out.size() && out[i + 1] == '\n') {
                eol.push_back('\n');
                ++i; // skip LF consumption by loop increment
            } else {
                eol.push_back('\n');
            }
        } else {
            eol.push_back(c);
        }
    }

    // Ensure file ends with a newline if not empty and not ending with backslash
    if (!eol.empty()) {
        if (eol.back() != '\n') {
            eol.push_back('\n');
        }
    }

    return eol;
}

} // namespace wvmcc
