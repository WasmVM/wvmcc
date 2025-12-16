#include "../../src/pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>

// Preprocess a source that includes a missing header and assert:
// - No HeaderName token is emitted
// - Diagnostics contain an error for missing include
int main() {
    using namespace wvmcc;

    const std::string srcName = "temp_source_failure.c";
    {
        std::ofstream ofs(srcName);
        ofs << "#include \"missing_header_12345.h\"\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "pp_include_failure_test: preprocess failed unexpectedly: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasHeaderName = false;
    for (const auto& t : res.tokens) {
        if (t.kind == PPTokenKind::HeaderName) hasHeaderName = true;
    }

    if (hasHeaderName) {
        std::cerr << "pp_include_failure_test: unexpected HeaderName token emitted on missing include\n";
        return 2;
    }

    const auto& diags = pp.getDiagnostics();
    bool hasError = false;
    for (const auto& d : diags) {
        if (d.severity == Preprocessor::Diagnostic::Severity::Error) {
            hasError = true;
            break;
        }
    }
    if (!hasError) {
        std::cerr << "pp_include_failure_test: no diagnostics Error reported for missing include\n";
        return 3;
    }
    return 0;
}
