#include "../../src/pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>

// Test 1: Successful include with quote syntax
static int test_include_success() {
    using namespace wvmcc;
    const std::string hdrName = "temp_header_success.h";
    const std::string srcName = "temp_source_success.c";
    
    {
        std::ofstream ofs(hdrName);
        ofs << "int x;\n";
    }
    {
        std::ofstream ofs(srcName);
        ofs << "#include \"" << hdrName << "\"\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    
    bool hasHeaderName = false, hasInt = false, hasX = false, hasSemi = false;
    for (const auto& t : res.tokens) {
        if (t.kind == wvmcc::PPTokenKind::HeaderName) hasHeaderName = true;
        if (t.lexeme == "int") hasInt = true;
        if (t.lexeme == "x") hasX = true;
        if (t.kind == wvmcc::PPTokenKind::Punctuator && t.lexeme == ";") hasSemi = true;
    }

    std::remove(hdrName.c_str());
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_include_success: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    if (hasHeaderName) {
        std::cerr << "test_include_success: unexpected HeaderName token\n";
        return 2;
    }
    if (!(hasInt && hasX && hasSemi)) {
        std::cerr << "test_include_success: missing expected tokens (int/x/;)\n";
        return 3;
    }
    return 0;
}

// Test 2: Failed include with missing header
static int test_include_failure() {
    using namespace wvmcc;
    const std::string srcName = "temp_source_failure.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#include \"missing_header_12345.h\"\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    // Should fail due to missing header
    if (res.success) {
        std::cerr << "test_include_failure: expected failure for missing header\n";
        return 1;
    }
    
    // Verify error message
    if (res.errorMsg.find("error") == std::string::npos && 
        res.errorMsg.find("failed") == std::string::npos) {
        std::cerr << "test_include_failure: error message missing detail: " << res.errorMsg << "\n";
        return 2;
    }
    return 0;
}

// Test 3: Nested includes (A includes B, source includes A)
static int test_include_nested() {
    using namespace wvmcc;
    const std::string hdrB = "temp_nested_b.h";
    const std::string hdrA = "temp_nested_a.h";
    const std::string srcName = "temp_nested_source.c";

    {
        std::ofstream ofs(hdrB);
        ofs << "int y;\n";
    }
    {
        std::ofstream ofs(hdrA);
        ofs << "#include \"" << hdrB << "\"\nint x;\n";
    }
    {
        std::ofstream ofs(srcName);
        ofs << "#include \"" << hdrA << "\"\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    
    bool hasY = false, hasX = false;
    int yPos = -1, xPos = -1;
    size_t idx = 0;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "y") { hasY = true; yPos = idx; }
        if (t.lexeme == "x") { hasX = true; xPos = idx; }
        ++idx;
    }

    std::remove(hdrB.c_str());
    std::remove(hdrA.c_str());
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_include_nested: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    if (!(hasX && hasY)) {
        std::cerr << "test_include_nested: missing expected identifiers (x and y)\n";
        return 2;
    }
    if (yPos >= xPos) {
        std::cerr << "test_include_nested: incorrect token order (y should appear before x)\n";
        return 3;
    }
    return 0;
}

// Test 4: Cyclic include (a.h includes b.h, b.h includes a.h)
static int test_include_cyclic() {
    using namespace wvmcc;
    const std::string hdrA = "temp_cyclic_a.h";
    const std::string hdrB = "temp_cyclic_b.h";
    const std::string srcName = "temp_cyclic_source.c";

    try {
        {
            std::ofstream ofs(hdrB);
            ofs << "#include \"" << hdrA << "\"\nint b;\n";
        }
        {
            std::ofstream ofs(hdrA);
            ofs << "#include \"" << hdrB << "\"\nint a;\n";
        }
        {
            std::ofstream ofs(srcName);
            ofs << "#include \"" << hdrA << "\"\n";
        }

        Preprocessor pp;
        auto res = pp.run(srcName);
        
        // Preprocess should fail due to cycle
        if (res.success) {
            std::cerr << "test_include_cyclic: expected failure but got success\n";
            std::remove(hdrA.c_str());
            std::remove(hdrB.c_str());
            std::remove(srcName.c_str());
            return 1;
        }
        
        // Verify diagnostics contain cycle error
        const auto& diags = pp.getDiagnostics();
        bool hasCycleError = false;
        for (const auto& d : diags) {
            if (d.severity == wvmcc::Preprocessor::Diagnostic::Severity::Error &&
                d.message.find("cyclic") != std::string::npos) {
                hasCycleError = true;
                break;
            }
        }
        
        std::remove(hdrA.c_str());
        std::remove(hdrB.c_str());
        std::remove(srcName.c_str());
        
        if (!hasCycleError) {
            std::cerr << "test_include_cyclic: no cyclic include error in diagnostics\n";
            return 2;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "test_include_cyclic: exception: " << e.what() << "\n";
        std::remove(hdrA.c_str());
        std::remove(hdrB.c_str());
        std::remove(srcName.c_str());
        return 3;
    }
}

int main() {
    int result = 0;
    
    result = test_include_success();
    if (result != 0) {
        std::cerr << "test_include_success failed with code " << result << "\n";
        return result;
    }

    result = test_include_failure();
    if (result != 0) {
        std::cerr << "test_include_failure failed with code " << result << "\n";
        return result;
    }

    result = test_include_nested();
    if (result != 0) {
        std::cerr << "test_include_nested failed with code " << result << "\n";
        return result;
    }

    result = test_include_cyclic();
    if (result != 0) {
        std::cerr << "test_include_cyclic failed with code " << result << "\n";
        return result;
    }

    return 0;
}
