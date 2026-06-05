#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cassert>
#include "pp/Preprocessor.hpp"

// Test 1: Simple #if with true condition
static int test_if_true() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_if_true.c");
        ofs << "#if 1\n";
        ofs << "int x;\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_if_true.c")) {
        std::remove("temp_if_true.c");
        std::cerr << "test_if_true: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_if_true.c");

    bool hasX = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (!hasX) {
        std::cerr << "test_if_true: expected 'x' identifier in output\n";
        return 2;
    }
    return 0;
}

// Test 2: Simple #if with false condition
static int test_if_false() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_if_false.c");
        ofs << "#if 0\n";
        ofs << "int x;\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_if_false.c")) {
        std::remove("temp_if_false.c");
        std::cerr << "test_if_false: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_if_false.c");

    bool hasX = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (hasX) {
        std::cerr << "test_if_false: expected NO 'x' identifier in output\n";
        return 2;
    }
    return 0;
}

// Test 3: #if #else
static int test_if_else() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_if_else.c");
        ofs << "#if 0\n";
        ofs << "int x;\n";
        ofs << "#else\n";
        ofs << "int y;\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_if_else.c")) {
        std::remove("temp_if_else.c");
        std::cerr << "test_if_else: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_if_else.c");

    bool hasX = false, hasY = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") hasX = true;
        if (t.lexeme == "y") hasY = true;
    }
    if (hasX) {
        std::cerr << "test_if_else: expected NO 'x' in output\n";
        return 2;
    }
    if (!hasY) {
        std::cerr << "test_if_else: expected 'y' in output\n";
        return 3;
    }
    return 0;
}

// Test 4: #ifdef
static int test_ifdef() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_ifdef.c");
        ofs << "#define MYVAR\n";
        ofs << "#ifdef MYVAR\n";
        ofs << "int x;\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_ifdef.c")) {
        std::remove("temp_ifdef.c");
        std::cerr << "test_ifdef: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_ifdef.c");

    bool hasX = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (!hasX) {
        std::cerr << "test_ifdef: expected 'x' in output\n";
        return 2;
    }
    return 0;
}

// Test 5: #ifndef
static int test_ifndef() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_ifndef.c");
        ofs << "#ifndef UNDEFINED\n";
        ofs << "int x;\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_ifndef.c")) {
        std::remove("temp_ifndef.c");
        std::cerr << "test_ifndef: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_ifndef.c");

    bool hasX = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (!hasX) {
        std::cerr << "test_ifndef: expected 'x' in output\n";
        std::cerr << "tokens:";
        for (const auto& t : tokens) {
            std::cerr << " [" << t.lexeme << "]";
        }
        std::cerr << "\n";
        return 2;
    }
    return 0;
}

// Test 6: #if #elif #else
static int test_elif() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_elif.c");
        ofs << "#if 0\n";
        ofs << "int x;\n";
        ofs << "#elif 1\n";
        ofs << "int y;\n";
        ofs << "#else\n";
        ofs << "int z;\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_elif.c")) {
        std::remove("temp_elif.c");
        std::cerr << "test_elif: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_elif.c");

    bool hasX = false, hasY = false, hasZ = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") hasX = true;
        if (t.lexeme == "y") hasY = true;
        if (t.lexeme == "z") hasZ = true;
    }
    if (hasX || hasZ) {
        std::cerr << "test_elif: expected only 'y', not x or z\n";
        return 2;
    }
    if (!hasY) {
        std::cerr << "test_elif: expected 'y' in output\n";
        return 3;
    }
    return 0;
}

// Test 7: Nested conditionals
static int test_nested() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_nested.c");
        ofs << "#if 1\n";
        ofs << "#if 1\n";
        ofs << "int x;\n";
        ofs << "#endif\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_nested.c")) {
        std::remove("temp_nested.c");
        std::cerr << "test_nested: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_nested.c");

    bool hasX = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (!hasX) {
        std::cerr << "test_nested: expected 'x' in output\n";
        return 2;
    }
    return 0;
}

// Test 8: defined() operator
static int test_defined() {
    using namespace wvmcc;
    
    {
        std::ofstream ofs("temp_defined.c");
        ofs << "#define MYVAR 1\n";
        ofs << "#if defined(MYVAR)\n";
        ofs << "int x;\n";
        ofs << "#endif\n";
    }

    Preprocessor pp;
    if (!pp.open("temp_defined.c")) {
        std::remove("temp_defined.c");
        std::cerr << "test_defined: failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove("temp_defined.c");

    bool hasX = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (!hasX) {
        std::cerr << "test_defined: expected 'x' in output\n";
        return 2;
    }
    return 0;
}

// Helper: write `src` to `path`, preprocess it, and report whether `needle`
// appears as a token lexeme in the output. Returns 0 on the expected result.
static int expect_token_presence(const char* path, const std::string& src,
                                 const std::string& needle, bool shouldBePresent,
                                 const char* testName) {
    using namespace wvmcc;
    {
        std::ofstream ofs(path);
        ofs << src;
    }
    Preprocessor pp;
    if (!pp.open(path)) {
        std::remove(path);
        std::cerr << testName << ": failed to open input\n";
        return 1;
    }
    std::vector<wvmcc::PPToken> tokens;
    while (auto t = pp.next()) tokens.push_back(*t);
    std::remove(path);

    bool present = false;
    for (const auto& t : tokens) {
        if (t.lexeme == needle) { present = true; break; }
    }
    if (present != shouldBePresent) {
        std::cerr << testName << ": expected '" << needle << "' to be "
                  << (shouldBePresent ? "present" : "absent")
                  << " but it was " << (present ? "present" : "absent") << "\n";
        return 2;
    }
    return 0;
}

// Test 9 (#80 follow-up): a true #if nested inside an INACTIVE block must stay
// inactive — the inner true condition must not re-activate output.
//   #if 0 / #if 1 / INNER / #endif / OUTER / #endif / AFTER  -> only AFTER
static int test_nested_if_in_inactive() {
    int r = expect_token_presence(
        "temp_nested_if_inactive.c",
        "#if 0\n#if 1\nINNER\n#endif\nOUTER\n#endif\nAFTER\n",
        "INNER", false, "test_nested_if_in_inactive (INNER absent)");
    if (r) return r;
    return expect_token_presence(
        "temp_nested_if_inactive2.c",
        "#if 0\n#if 1\nINNER\n#endif\nOUTER\n#endif\nAFTER\n",
        "AFTER", true, "test_nested_if_in_inactive (AFTER present)");
}

// Test 10: a true #elif nested inside an INACTIVE block must stay inactive.
//   #if 0 / #if 0 / #elif 1 / X / #endif / #endif / AFTER  -> only AFTER
static int test_nested_elif_in_inactive() {
    int r = expect_token_presence(
        "temp_nested_elif_inactive.c",
        "#if 0\n#if 0\n#elif 1\nX\n#endif\n#endif\nAFTER\n",
        "X", false, "test_nested_elif_in_inactive (X absent)");
    if (r) return r;
    return expect_token_presence(
        "temp_nested_elif_inactive2.c",
        "#if 0\n#if 0\n#elif 1\nX\n#endif\n#endif\nAFTER\n",
        "AFTER", true, "test_nested_elif_in_inactive (AFTER present)");
}

// Test 11: a #else nested inside an INACTIVE block must stay inactive.
//   #if 0 / #if 0 / #else / Y / #endif / #endif / AFTER  -> only AFTER
static int test_nested_else_in_inactive() {
    int r = expect_token_presence(
        "temp_nested_else_inactive.c",
        "#if 0\n#if 0\n#else\nY\n#endif\n#endif\nAFTER\n",
        "Y", false, "test_nested_else_in_inactive (Y absent)");
    if (r) return r;
    return expect_token_presence(
        "temp_nested_else_inactive2.c",
        "#if 0\n#if 0\n#else\nY\n#endif\n#endif\nAFTER\n",
        "AFTER", true, "test_nested_else_in_inactive (AFTER present)");
}

// Test 12: nested conditionals inside an ACTIVE block still work (guard against
// over-correcting the parent-awareness fix).
//   #if 1 / #if 1 / INNER / #endif / #if 0 / #else / ELSE / #endif / #endif
static int test_nested_in_active() {
    int r = expect_token_presence(
        "temp_nested_active.c",
        "#if 1\n#if 1\nINNER\n#endif\n#if 0\n#else\nELSE\n#endif\n#endif\n",
        "INNER", true, "test_nested_in_active (INNER present)");
    if (r) return r;
    return expect_token_presence(
        "temp_nested_active2.c",
        "#if 1\n#if 1\nINNER\n#endif\n#if 0\n#else\nELSE\n#endif\n#endif\n",
        "ELSE", true, "test_nested_in_active (ELSE present)");
}

int main() {
    int result;

    result = test_if_true();
    if (result != 0) {
        std::cerr << "test_if_true failed with code " << result << "\n";
        return result;
    }

    result = test_if_false();
    if (result != 0) {
        std::cerr << "test_if_false failed with code " << result << "\n";
        return result;
    }

    result = test_if_else();
    if (result != 0) {
        std::cerr << "test_if_else failed with code " << result << "\n";
        return result;
    }

    result = test_ifdef();
    if (result != 0) {
        std::cerr << "test_ifdef failed with code " << result << "\n";
        return result;
    }

    result = test_ifndef();
    if (result != 0) {
        std::cerr << "test_ifndef failed with code " << result << "\n";
        return result;
    }

    result = test_elif();
    if (result != 0) {
        std::cerr << "test_elif failed with code " << result << "\n";
        return result;
    }

    result = test_nested();
    if (result != 0) {
        std::cerr << "test_nested failed with code " << result << "\n";
        return result;
    }

    result = test_defined();
    if (result != 0) {
        std::cerr << "test_defined failed with code " << result << "\n";
        return result;
    }

    result = test_nested_if_in_inactive();
    if (result != 0) {
        std::cerr << "test_nested_if_in_inactive failed with code " << result << "\n";
        return result;
    }

    result = test_nested_elif_in_inactive();
    if (result != 0) {
        std::cerr << "test_nested_elif_in_inactive failed with code " << result << "\n";
        return result;
    }

    result = test_nested_else_in_inactive();
    if (result != 0) {
        std::cerr << "test_nested_else_in_inactive failed with code " << result << "\n";
        return result;
    }

    result = test_nested_in_active();
    if (result != 0) {
        std::cerr << "test_nested_in_active failed with code " << result << "\n";
        return result;
    }

    std::cout << "pp_conditional_test: all cases passed\n";
    return 0;
}
