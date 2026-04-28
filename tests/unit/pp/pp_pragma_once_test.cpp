#include "pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

using namespace wvmcc;

// Test helper: create a header file with #pragma once
void createHeaderWithPragmaOnce(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    out << "#pragma once\n";
    out << content << "\n";
    out.close();
}

// Test helper: create a header file without #pragma once
void createHeaderNoGuard(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    out << content << "\n";
    out.close();
}

// Test 1: Basic #pragma once functionality
static int test_basic_pragma_once() {
    // Create header with #pragma once
    createHeaderWithPragmaOnce("test_once1.h", "int value = 42;");
    
    // Include it twice
    std::string source = R"(
#include "test_once1.h"
#include "test_once1.h"
)";
    
    std::ofstream out("test_once_main1.c");
    out << source;
    out.close();
    
    Preprocessor pp;
    if (!pp.open("test_once_main1.c")) {
        std::filesystem::remove("test_once1.h");
        std::filesystem::remove("test_once_main1.c");
        std::cerr << "test_basic_pragma_once: failed to open input\n";
        return 1;
    }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    
    // Clean up
    std::filesystem::remove("test_once1.h");
    std::filesystem::remove("test_once_main1.c");
    
    if (pp.getDiagnostics().end() != std::find_if(pp.getDiagnostics().begin(), pp.getDiagnostics().end(), [](const Diagnostic& d){ return d.severity==Diagnostic::Severity::Error; })) {
        std::cerr << "Preprocessing failed" << std::endl;
        return 1;
    }
    
    // Count how many times "value" appears
    int valueCount = 0;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "value") {
            valueCount++;
        }
    }
    
    // Should only appear once due to #pragma once
    if (valueCount != 1) {
        std::cerr << "Expected 'value' to appear once, got " << valueCount << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 2: No #pragma once means reprocessing
static int test_no_pragma_once() {
    // Create header WITHOUT #pragma once
    createHeaderNoGuard("test_once2.h", "int counter = 0;");
    
    // Include it twice
    std::string source = R"(
#include "test_once2.h"
#include "test_once2.h"
)";
    
    std::ofstream out("test_once_main2.c");
    out << source;
    out.close();
    
    Preprocessor pp;
    if (!pp.open("test_once_main2.c")) {
        std::filesystem::remove("test_once2.h");
        std::filesystem::remove("test_once_main2.c");
        std::cerr << "test_no_pragma_once: failed to open input\n";
        return 1;
    }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    
    // Clean up
    std::filesystem::remove("test_once2.h");
    std::filesystem::remove("test_once_main2.c");
    
    if (pp.getDiagnostics().end() != std::find_if(pp.getDiagnostics().begin(), pp.getDiagnostics().end(), [](const Diagnostic& d){ return d.severity==Diagnostic::Severity::Error; })) {
        std::cerr << "Preprocessing failed" << std::endl;
        return 1;
    }
    
    // Count how many times "counter" appears
    int counterCount = 0;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "counter") {
            counterCount++;
        }
    }
    
    // Should appear twice (no #pragma once, so both includes are processed)
    if (counterCount != 2) {
        std::cerr << "Expected 'counter' to appear twice, got " << counterCount << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 3: Multiple headers with #pragma once
static int test_multiple_pragma_once() {
    // Create two headers with #pragma once
    createHeaderWithPragmaOnce("test_once3a.h", "int a = 1;");
    createHeaderWithPragmaOnce("test_once3b.h", "int b = 2;");
    
    // Include them multiple times
    std::string source = R"(
#include "test_once3a.h"
#include "test_once3b.h"
#include "test_once3a.h"
#include "test_once3b.h"
)";
    
    std::ofstream out("test_once_main3.c");
    out << source;
    out.close();
    
    Preprocessor pp;
    if (!pp.open("test_once_main3.c")) {
        std::filesystem::remove("test_once3a.h");
        std::filesystem::remove("test_once3b.h");
        std::filesystem::remove("test_once_main3.c");
        std::cerr << "test_multiple_pragma_once: failed to open input\n";
        return 1;
    }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    
    // Clean up
    std::filesystem::remove("test_once3a.h");
    std::filesystem::remove("test_once3b.h");
    std::filesystem::remove("test_once_main3.c");
    
    if (pp.getDiagnostics().end() != std::find_if(pp.getDiagnostics().begin(), pp.getDiagnostics().end(), [](const Diagnostic& d){ return d.severity==Diagnostic::Severity::Error; })) {
        std::cerr << "Preprocessing failed" << std::endl;
        return 1;
    }
    
    // Count identifiers
    int aCount = 0, bCount = 0;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Identifier) {
            if (tok.lexeme == "a") aCount++;
            if (tok.lexeme == "b") bCount++;
        }
    }
    
    // Each should appear only once
    if (aCount != 1 || bCount != 1) {
        std::cerr << "Expected a=1, b=1, got a=" << aCount << ", b=" << bCount << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 4: Cross-file includes with #pragma once
static int test_cross_file_pragma_once() {
    // Create headers that include each other with #pragma once
    std::ofstream out1("test_once4a.h");
    out1 << "#pragma once\n";
    out1 << "#include \"test_once4b.h\"\n";
    out1 << "int a = 1;\n";
    out1.close();
    
    std::ofstream out2("test_once4b.h");
    out2 << "#pragma once\n";
    out2 << "#include \"test_once4a.h\"\n";
    out2 << "int b = 2;\n";
    out2.close();
    
    // Include both in main file
    std::string source = R"(
#include "test_once4a.h"
#include "test_once4b.h"
)";
    
    std::ofstream out("test_once_main4.c");
    out << source;
    out.close();
    
    Preprocessor pp;
    if (!pp.open("test_once_main4.c")) {
        std::filesystem::remove("test_once4a.h");
        std::filesystem::remove("test_once4b.h");
        std::filesystem::remove("test_once_main4.c");
        std::cerr << "test_cross_file_pragma_once: failed to open input\n";
        return 1;
    }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    
    // Clean up
    std::filesystem::remove("test_once4a.h");
    std::filesystem::remove("test_once4b.h");
    std::filesystem::remove("test_once_main4.c");
    
    if (pp.getDiagnostics().end() != std::find_if(pp.getDiagnostics().begin(), pp.getDiagnostics().end(), [](const Diagnostic& d){ return d.severity==Diagnostic::Severity::Error; })) {
        std::cerr << "Preprocessing failed" << std::endl;
        return 1;
    }
    
    // Should successfully handle mutual inclusion
    int aCount = 0, bCount = 0;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Identifier) {
            if (tok.lexeme == "a") aCount++;
            if (tok.lexeme == "b") bCount++;
        }
    }
    
    // Each should appear once
    if (aCount != 1 || bCount != 1) {
        std::cerr << "Expected a=1, b=1 for mutual inclusion, got a=" << aCount << ", b=" << bCount << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 5: #pragma once with complex content
static int test_pragma_once_complex() {
    // Create header with more complex content
    std::ofstream out("test_once5.h");
    out << "#pragma once\n";
    out << "\n";
    out << "typedef struct {\n";
    out << "    int x;\n";
    out << "    int y;\n";
    out << "} Point;\n";
    out << "\n";
    out << "void init_point(Point *p);\n";
    out << "\n";
    out.close();
    
    // Include multiple times
    std::string source = R"(
#include "test_once5.h"
#include "test_once5.h"
#include "test_once5.h"
)";
    
    std::ofstream outmain("test_once_main5.c");
    outmain << source;
    outmain.close();
    
    Preprocessor pp;
    if (!pp.open("test_once_main5.c")) {
        std::filesystem::remove("test_once5.h");
        std::filesystem::remove("test_once_main5.c");
        std::cerr << "test_pragma_once_complex: failed to open input\n";
        return 1;
    }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    
    // Clean up
    std::filesystem::remove("test_once5.h");
    std::filesystem::remove("test_once_main5.c");
    
    if (pp.getDiagnostics().end() != std::find_if(pp.getDiagnostics().begin(), pp.getDiagnostics().end(), [](const Diagnostic& d){ return d.severity==Diagnostic::Severity::Error; })) {
        std::cerr << "Preprocessing failed" << std::endl;
        return 1;
    }
    
    // Count "Point" identifier
    int pointCount = 0;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "Point") {
            pointCount++;
        }
    }
    
    // Should appear only twice (typedef and parameter type) from single processing
    if (pointCount != 2) {
        std::cerr << "Expected 'Point' to appear twice, got " << pointCount << std::endl;
        return 1;
    }
    
    return 0;
}

// Test 6: #pragma once only affects file it's in
static int test_pragma_once_scoping() {
    // Create two files with same content but different names, both with #pragma once
    createHeaderWithPragmaOnce("test_once6a.h", "int shared = 100;");
    createHeaderWithPragmaOnce("test_once6b.h", "int shared = 100;");
    
    // Include both (should both be processed since they're different files)
    std::string source = R"(
#include "test_once6a.h"
#include "test_once6b.h"
)";
    
    std::ofstream out("test_once_main6.c");
    out << source;
    out.close();
    
    Preprocessor pp;
    if (!pp.open("test_once_main6.c")) {
        std::filesystem::remove("test_once6a.h");
        std::filesystem::remove("test_once6b.h");
        std::filesystem::remove("test_once_main6.c");
        std::cerr << "test_pragma_once_scoping: failed to open input\n";
        return 1;
    }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    
    // Clean up
    std::filesystem::remove("test_once6a.h");
    std::filesystem::remove("test_once6b.h");
    std::filesystem::remove("test_once_main6.c");
    
    if (pp.getDiagnostics().end() != std::find_if(pp.getDiagnostics().begin(), pp.getDiagnostics().end(), [](const Diagnostic& d){ return d.severity==Diagnostic::Severity::Error; })) {
        std::cerr << "Preprocessing failed" << std::endl;
        return 1;
    }
    
    // Count how many times "shared" appears
    int sharedCount = 0;
    for (const auto& tok : tokens) {
        if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "shared") {
            sharedCount++;
        }
    }
    
    // Should appear twice (once from each file)
    if (sharedCount != 2) {
        std::cerr << "Expected 'shared' to appear twice, got " << sharedCount << std::endl;
        return 1;
    }
    
    return 0;
}

int main() {
    int failed = 0;
    
    std::cout << "Running test_basic_pragma_once..." << std::endl;
    if (test_basic_pragma_once() != 0) {
        std::cerr << "test_basic_pragma_once FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_basic_pragma_once PASSED" << std::endl;
    }
    
    std::cout << "Running test_no_pragma_once..." << std::endl;
    if (test_no_pragma_once() != 0) {
        std::cerr << "test_no_pragma_once FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_no_pragma_once PASSED" << std::endl;
    }
    
    std::cout << "Running test_multiple_pragma_once..." << std::endl;
    if (test_multiple_pragma_once() != 0) {
        std::cerr << "test_multiple_pragma_once FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_multiple_pragma_once PASSED" << std::endl;
    }
    
    std::cout << "Running test_cross_file_pragma_once..." << std::endl;
    if (test_cross_file_pragma_once() != 0) {
        std::cerr << "test_cross_file_pragma_once FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_cross_file_pragma_once PASSED" << std::endl;
    }
    
    std::cout << "Running test_pragma_once_complex..." << std::endl;
    if (test_pragma_once_complex() != 0) {
        std::cerr << "test_pragma_once_complex FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_pragma_once_complex PASSED" << std::endl;
    }
    
    std::cout << "Running test_pragma_once_scoping..." << std::endl;
    if (test_pragma_once_scoping() != 0) {
        std::cerr << "test_pragma_once_scoping FAILED" << std::endl;
        failed++;
    } else {
        std::cout << "test_pragma_once_scoping PASSED" << std::endl;
    }
    
    if (failed == 0) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n" << failed << " test(s) failed!" << std::endl;
        return 1;
    }
}
