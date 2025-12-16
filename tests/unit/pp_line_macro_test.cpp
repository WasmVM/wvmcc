#include "../../src/pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

using namespace wvmcc;

// Test helper: preprocess a string and return tokens
std::vector<PPToken> preprocessString(const std::string& source, const std::string& filename = "test.c") {
    // Write to temporary file
    std::ofstream out(filename);
    out << source;
    out.close();
    Preprocessor pp;
    if (!pp.open(filename)) {
        std::filesystem::remove(filename);
        return {};
    }

    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);

    // Clean up
    std::filesystem::remove(filename);
    return tokens;
}

// Test 1: Basic __LINE__ expansion
static int test_basic_line_expansion() {
    std::string source = R"(
int x = __LINE__;
int y = __LINE__;
int z = __LINE__;
)";
    
    auto tokens = preprocessString(source);
    
    // Find the three numeric literals (line numbers)
    std::vector<std::string> lineNumbers;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::PPNumber) {
            lineNumbers.push_back(tok.lexeme);
        }
    }
    
    if (lineNumbers.size() != 3) {
        std::cerr << "Expected 3 line numbers, got " << lineNumbers.size() << std::endl;
        return 1;
    }
    if (lineNumbers[0] != "2" || lineNumbers[1] != "3" || lineNumbers[2] != "4") {
        std::cerr << "Line numbers incorrect: " << lineNumbers[0] << ", " << lineNumbers[1] << ", " << lineNumbers[2] << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 2: __LINE__ in macro definition
static int test_line_in_macro() {
    std::string source = R"(
#define REPORT_LINE __LINE__
int a = REPORT_LINE;
int b = REPORT_LINE;
int c = REPORT_LINE;
)";
    
    auto tokens = preprocessString(source);
    
    // Find the three numeric literals
    std::vector<std::string> lineNumbers;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::PPNumber) {
            lineNumbers.push_back(tok.lexeme);
        }
    }
    
    if (lineNumbers.size() != 3) {
        std::cerr << "Expected 3 line numbers, got " << lineNumbers.size() << std::endl;
        return 1;
    }
    // Each should expand to the line where REPORT_LINE is used
    if (lineNumbers[0] != "3" || lineNumbers[1] != "4" || lineNumbers[2] != "5") {
        std::cerr << "Line numbers incorrect: " << lineNumbers[0] << ", " << lineNumbers[1] << ", " << lineNumbers[2] << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 3: __LINE__ in function-like macro
static int test_line_in_function_macro() {
    std::string source = R"(
#define LOG(msg) { int line = __LINE__; }
void test() {
    LOG("message1");
    LOG("message2");
}
)";
    
    auto tokens = preprocessString(source);
    
    // Find numeric literals
    std::vector<std::string> lineNumbers;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::PPNumber) {
            lineNumbers.push_back(tok.lexeme);
        }
    }
    
    // __LINE__ should expand to the line where LOG is invoked
    if (lineNumbers.size() < 2) {
        std::cerr << "Expected at least 2 line numbers, got " << lineNumbers.size() << std::endl;
        return 1;
    }
    if (lineNumbers[0] != "4" || lineNumbers[1] != "5") {
        std::cerr << "Line numbers incorrect: " << lineNumbers[0] << ", " << lineNumbers[1] << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 4: __LINE__ with conditionals
static int test_line_with_conditionals() {
    std::string source = R"(
#if 1
int x = __LINE__;
#endif
#if 0
int y = __LINE__;
#endif
int z = __LINE__;
)";
    
    auto tokens = preprocessString(source);
    
    // Find numeric literals
    std::vector<std::string> lineNumbers;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::PPNumber) {
            lineNumbers.push_back(tok.lexeme);
        }
    }
    
    // Should have two line numbers (lines 3 and 8, line 6 is in inactive region)
    if (lineNumbers.size() != 2) {
        std::cerr << "Expected 2 line numbers, got " << lineNumbers.size() << std::endl;
        return 1;
    }
    if (lineNumbers[0] != "3" || lineNumbers[1] != "8") {
        std::cerr << "Line numbers incorrect: " << lineNumbers[0] << ", " << lineNumbers[1] << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 5: Multiple __LINE__ on same line
static int test_multiple_line_same_line() {
    std::string source = "int x = __LINE__, y = __LINE__, z = __LINE__;\n";
    
    auto tokens = preprocessString(source);
    
    // Find numeric literals
    std::vector<std::string> lineNumbers;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::PPNumber) {
            lineNumbers.push_back(tok.lexeme);
        }
    }
    
    // All should expand to line 1
    if (lineNumbers.size() != 3) {
        std::cerr << "Expected 3 line numbers, got " << lineNumbers.size() << std::endl;
        return 1;
    }
    if (lineNumbers[0] != "1" || lineNumbers[1] != "1" || lineNumbers[2] != "1") {
        std::cerr << "Line numbers incorrect: " << lineNumbers[0] << ", " << lineNumbers[1] << ", " << lineNumbers[2] << std::endl;
        return 1;
    }
    
    return 0;
}

int main() {
    int failed = 0;
    
    std::cout << "Running test_basic_line_expansion..." << std::endl;
    if (test_basic_line_expansion() != 0) {
        std::cerr << "test_basic_line_expansion FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_basic_line_expansion PASSED" << std::endl;
    }
    
    std::cout << "Running test_line_in_macro..." << std::endl;
    if (test_line_in_macro() != 0) {
        std::cerr << "test_line_in_macro FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_line_in_macro PASSED" << std::endl;
    }
    
    std::cout << "Running test_line_in_function_macro..." << std::endl;
    if (test_line_in_function_macro() != 0) {
        std::cerr << "test_line_in_function_macro FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_line_in_function_macro PASSED" << std::endl;
    }
    
    std::cout << "Running test_line_with_conditionals..." << std::endl;
    if (test_line_with_conditionals() != 0) {
        std::cerr << "test_line_with_conditionals FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_line_with_conditionals PASSED" << std::endl;
    }
    
    std::cout << "Running test_multiple_line_same_line..." << std::endl;
    if (test_multiple_line_same_line() != 0) {
        std::cerr << "test_multiple_line_same_line FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_multiple_line_same_line PASSED" << std::endl;
    }
    
    if (failed == 0) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n" << failed << " test(s) failed!" << std::endl;
        return 1;
    }
}
