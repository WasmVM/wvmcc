#include "pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>

// Test 1: #error directive
static int test_error_directive() {
    using namespace wvmcc;
    const std::string srcName = "temp_directive_error.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#error This is an error message\n";
        ofs << "int x = 1;\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) { std::remove(srcName.c_str()); std::cerr << "test_error_directive: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(srcName.c_str());

    // #error should cause a diagnostic with severity Error
    const auto& diags = pp.getDiagnostics();
    bool foundError = false;
    for (const auto& d : diags) {
        if (d.severity == Diagnostic::Severity::Error && 
            d.message.find("#error:") != std::string::npos) {
            foundError = true;
            break;
        }
    }
    if (!foundError) {
        std::cerr << "test_error_directive: expected #error diagnostic\n";
        return 2;
    }
    
    return 0;
}

// Test 2: #warning directive
static int test_warning_directive() {
    using namespace wvmcc;
    const std::string srcName = "temp_directive_warning.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#warning This is a warning message\n";
        ofs << "int x = 1;\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) { std::remove(srcName.c_str()); std::cerr << "test_warning_directive: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(srcName.c_str());

    // Check that warning diagnostic was generated
    const auto& diags = pp.getDiagnostics();
    bool foundWarning = false;
    for (const auto& d : diags) {
        if (d.severity == Diagnostic::Severity::Warning && 
            d.message.find("#warning:") != std::string::npos) {
            foundWarning = true;
            break;
        }
    }
    
    if (!foundWarning) {
        std::cerr << "test_warning_directive: expected #warning diagnostic\n";
        return 2;
    }
    
    return 0;
}

// Test 3: #line directive with line number only
static int test_line_directive_simple() {
    using namespace wvmcc;
    const std::string srcName = "temp_directive_line.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#line 100\n";
        ofs << "int x = 1;\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) { std::remove(srcName.c_str()); std::cerr << "test_line_directive_simple: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(srcName.c_str());
    // Ensure no error diagnostics
    for (const auto& d : pp.getDiagnostics()) { if (d.severity == Diagnostic::Severity::Error) { std::cerr << "test_line_directive_simple: unexpected error diagnostic\n"; return 1; } }
    
    return 0;
}

// Test 4: #line directive with line number and filename
static int test_line_directive_filename() {
    using namespace wvmcc;
    const std::string srcName = "temp_directive_line_file.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#line 200 \"custom.c\"\n";
        ofs << "int y = 2;\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) { std::remove(srcName.c_str()); std::cerr << "test_line_directive_filename: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(srcName.c_str());
    for (const auto& d : pp.getDiagnostics()) { if (d.severity == Diagnostic::Severity::Error) { std::cerr << "test_line_directive_filename: unexpected error diagnostic\n"; return 1; } }
    
    return 0;
}

// Test 5: #pragma directive
static int test_pragma_directive() {
    using namespace wvmcc;
    const std::string srcName = "temp_directive_pragma.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#pragma unknown_directive\n";
        ofs << "int z = 3;\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) { std::remove(srcName.c_str()); std::cerr << "test_pragma_directive: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(srcName.c_str());

    // Check that pragma diagnostic was generated (for unknown pragmas, not "once")
    const auto& diags = pp.getDiagnostics();
    bool foundPragma = false;
    for (const auto& d : diags) {
        if (d.message.find("#pragma:") != std::string::npos) {
            foundPragma = true;
            break;
        }
    }
    
    if (!foundPragma) {
        std::cerr << "test_pragma_directive: expected #pragma diagnostic\n";
        return 2;
    }
    
    return 0;
}

// Test 6: #error with conditional (inactive)
static int test_error_conditional_inactive() {
    using namespace wvmcc;
    const std::string srcName = "temp_directive_error_cond.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#if 0\n";
        ofs << "#error This should not trigger\n";
        ofs << "#endif\n";
        ofs << "int x = 1;\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) { std::remove(srcName.c_str()); std::cerr << "test_error_conditional_inactive: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(srcName.c_str());
    for (const auto& d : pp.getDiagnostics()) { if (d.severity == Diagnostic::Severity::Error) { std::cerr << "test_error_conditional_inactive: unexpected error diagnostic\n"; return 1; } }
    
    return 0;
}

// Test 7: #error with conditional (active)
static int test_error_conditional_active() {
    using namespace wvmcc;
    const std::string srcName = "temp_directive_error_active.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#if 1\n";
        ofs << "#error This should trigger\n";
        ofs << "#endif\n";
        ofs << "int x = 1;\n";
    }

    Preprocessor pp;
    if (!pp.open(srcName)) { std::remove(srcName.c_str()); std::cerr << "test_error_conditional_active: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(srcName.c_str());
    bool foundError = false;
    for (const auto& d : pp.getDiagnostics()) { if (d.severity == Diagnostic::Severity::Error) { foundError = true; break; } }
    if (!foundError) { std::cerr << "test_error_conditional_active: expected error diagnostic\n"; return 1; }
    
    return 0;
}

int main() {
    int result = 0;

    result = test_error_directive();
    if (result != 0) {
        std::cerr << "test_error_directive failed with code " << result << "\n";
        return result;
    }

    result = test_warning_directive();
    if (result != 0) {
        std::cerr << "test_warning_directive failed with code " << result << "\n";
        return result;
    }

    result = test_line_directive_simple();
    if (result != 0) {
        std::cerr << "test_line_directive_simple failed with code " << result << "\n";
        return result;
    }

    result = test_line_directive_filename();
    if (result != 0) {
        std::cerr << "test_line_directive_filename failed with code " << result << "\n";
        return result;
    }

    result = test_pragma_directive();
    if (result != 0) {
        std::cerr << "test_pragma_directive failed with code " << result << "\n";
        return result;
    }

    result = test_error_conditional_inactive();
    if (result != 0) {
        std::cerr << "test_error_conditional_inactive failed with code " << result << "\n";
        return result;
    }

    result = test_error_conditional_active();
    if (result != 0) {
        std::cerr << "test_error_conditional_active failed with code " << result << "\n";
        return result;
    }

    return 0;
}
