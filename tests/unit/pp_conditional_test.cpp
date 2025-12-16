#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cassert>
#include "../src/pp/Preprocessor.hpp"

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
    auto res = pp.run("temp_if_true.c");
    std::remove("temp_if_true.c");

    if (!res.success) {
        std::cerr << "test_if_true: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false;
    for (const auto& t : res.tokens) {
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
    auto res = pp.run("temp_if_false.c");
    std::remove("temp_if_false.c");

    if (!res.success) {
        std::cerr << "test_if_false: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false;
    for (const auto& t : res.tokens) {
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
    auto res = pp.run("temp_if_else.c");
    std::remove("temp_if_else.c");

    if (!res.success) {
        std::cerr << "test_if_else: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false, hasY = false;
    for (const auto& t : res.tokens) {
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
    auto res = pp.run("temp_ifdef.c");
    std::remove("temp_ifdef.c");

    if (!res.success) {
        std::cerr << "test_ifdef: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false;
    for (const auto& t : res.tokens) {
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
    auto res = pp.run("temp_ifndef.c");
    std::remove("temp_ifndef.c");

    if (!res.success) {
        std::cerr << "test_ifndef: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (!hasX) {
        std::cerr << "test_ifndef: expected 'x' in output\n";
        std::cerr << "tokens:";
        for (const auto& t : res.tokens) {
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
    auto res = pp.run("temp_elif.c");
    std::remove("temp_elif.c");

    if (!res.success) {
        std::cerr << "test_elif: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false, hasY = false, hasZ = false;
    for (const auto& t : res.tokens) {
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
    auto res = pp.run("temp_nested.c");
    std::remove("temp_nested.c");

    if (!res.success) {
        std::cerr << "test_nested: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false;
    for (const auto& t : res.tokens) {
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
    auto res = pp.run("temp_defined.c");
    std::remove("temp_defined.c");

    if (!res.success) {
        std::cerr << "test_defined: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    bool hasX = false;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "x") { hasX = true; break; }
    }
    if (!hasX) {
        std::cerr << "test_defined: expected 'x' in output\n";
        return 2;
    }
    return 0;
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

    std::cout << "pp_conditional_test: all cases passed\n";
    return 0;
}
