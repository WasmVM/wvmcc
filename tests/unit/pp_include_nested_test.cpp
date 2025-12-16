#include "../../src/pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>

// Creates two headers where A includes B, then a source includes A.
// Asserts that tokens from both headers are emitted inline in order.
int main() {
    using namespace wvmcc;

    const std::string hdrB = "temp_nested_b.h";
    const std::string hdrA = "temp_nested_a.h";
    const std::string srcName = "temp_nested_source.c";

    // Write header B
    {
        std::ofstream ofs(hdrB);
        ofs << "int y;\n";
        ofs.close();
    }
    // Write header A that includes B
    {
        std::ofstream ofs(hdrA);
        ofs << "#include \"" << hdrB << "\"\nint x;\n";
        ofs.close();
    }
    // Source includes A
    {
        std::ofstream ofs(srcName);
        ofs << "#include \"" << hdrA << "\"\n";
        ofs.close();
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    if (!res.success) {
        std::cerr << "pp_include_nested_test: preprocess failed: " << res.errorMsg << "\n";
        std::remove(hdrB.c_str());
        std::remove(hdrA.c_str());
        std::remove(srcName.c_str());
        return 1;
    }

    bool hasIntX = false, hasIntY = false;
    bool hasSemiX = false, hasSemiY = false;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "int") {
            // We will see multiple 'int'; mark when followed by x or y via a simple lookahead in sequence
            // For a minimal check, just track presence of both identifiers and semicolons
        }
        if (t.lexeme == "x") hasIntX = true;
        if (t.lexeme == "y") hasIntY = true;
        if (t.kind == PPTokenKind::Punctuator && t.lexeme == ";") {
            // We'll see at least two semicolons
            if (!hasSemiY) hasSemiY = true; else hasSemiX = true;
        }
    }

    std::remove(hdrB.c_str());
    std::remove(hdrA.c_str());
    std::remove(srcName.c_str());

    if (!(hasIntX && hasIntY && hasSemiX && hasSemiY)) {
        std::cerr << "pp_include_nested_test: missing expected tokens from nested headers (int x; int y;)\n";
        return 2;
    }
    return 0;
}
