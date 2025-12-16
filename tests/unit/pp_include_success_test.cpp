#include "../../src/pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>

// Creates a temporary header file, then preprocesses a source that includes it.
// Asserts that tokens from the header are emitted inline and no HeaderName token is produced.
int main() {
    using namespace wvmcc;

    // Create a temporary header in the current directory
    const std::string hdrName = "temp_header_success.h";
    {
        std::ofstream ofs(hdrName);
        ofs << "int x;\n"; // simple content
        ofs.close();
    }

    // Create a temporary source that includes the header with quotes
    const std::string srcName = "temp_source_success.c";
    {
        std::ofstream ofs(srcName);
        ofs << "#include \"" << hdrName << "\"\n";
        ofs.close();
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    if (!res.success) {
        std::cerr << "pp_include_success_test: preprocess failed: " << res.errorMsg << "\n";
        std::remove(hdrName.c_str());
        std::remove(srcName.c_str());
        return 1;
    }

    // Expect no HeaderName token, and presence of tokens from header content (identifier 'int', identifier 'x', punct ';')
    bool hasHeaderName = false;
    bool hasInt = false, hasX = false, hasSemi = false;
    for (const auto& t : res.tokens) {
        if (t.kind == PPTokenKind::HeaderName) hasHeaderName = true;
        if (t.lexeme == "int") hasInt = true;
        if (t.lexeme == "x") hasX = true;
        if (t.kind == PPTokenKind::Punctuator && t.lexeme == ";") hasSemi = true;
    }

    // Clean up
    std::remove(hdrName.c_str());
    std::remove(srcName.c_str());

    if (hasHeaderName) {
        std::cerr << "pp_include_success_test: unexpected HeaderName token emitted\n";
        return 2;
    }
    if (!(hasInt && hasX && hasSemi)) {
        std::cerr << "pp_include_success_test: missing expected tokens from header (int/x/;)\n";
        return 3;
    }
    return 0;
}
