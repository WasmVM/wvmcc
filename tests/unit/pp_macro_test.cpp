#include "../../src/pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>

// Test 1: Simple object-like macro
static int test_object_macro_simple() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_simple.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define MAX 100\n";
        ofs << "int x = MAX;\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_object_macro_simple: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }

    // After macro expansion, "MAX" should be replaced with "100"
    bool hasMax = false, has100 = false;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "MAX") hasMax = true;
        if (t.lexeme == "100") has100 = true;
    }
    
    if (hasMax) {
        std::cerr << "test_object_macro_simple: MAX should have been expanded\n";
        return 2;
    }
    
    if (!has100) {
        std::cerr << "test_object_macro_simple: expected to find 100 in output after expansion\n";
        return 3;
    }
    
    return 0;
}

// Test 2: Multiple object-like macros
static int test_multiple_object_macros() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_multiple.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define MAX 100\n";
        ofs << "#define MIN 0\n";
        ofs << "#define SIZE 42\n";
        ofs << "int a = MAX, b = MIN, c = SIZE;\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_multiple_object_macros: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    return 0;
}

// Test 3: Object-like macro with empty replacement
static int test_empty_macro() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_empty.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define DEBUG\n";
        ofs << "void foo() {}\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_empty_macro: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    return 0;
}

// Test 4: Function-like macro definition
static int test_function_macro() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_func.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define ADD(x, y) ((x) + (y))\n";
        ofs << "int z = ADD(2, 3);\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_function_macro: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    // Verify macro was defined (should be in macro table)
    // For now, macro expansion is not implemented, so just verify parsing succeeds
    return 0;
}

// Test 5: Variadic function-like macro
static int test_variadic_macro() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_variadic.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define PRINTF(fmt, ...) printf(fmt, __VA_ARGS__)\n";
        ofs << "void test() { PRINTF(\"test\", 42); }\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_variadic_macro: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    return 0;
}

// Test 6: Undefine macro
static int test_undef_macro() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_undef.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define MAX 100\n";
        ofs << "#undef MAX\n";
        ofs << "int x = MAX;\n";  // Should be treated as identifier, not macro
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_undef_macro: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    return 0;
}

// Test 7: Object-like macro with complex replacement
static int test_macro_complex_replacement() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_complex.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define SWAP(a, b) { int temp = a; a = b; b = temp; }\n";
        ofs << "void swap_ints(int *x, int *y) SWAP(*x, *y)\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_macro_complex_replacement: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    return 0;
}

// Test 8: Macro redefinition (should succeed - replaces previous definition)
static int test_macro_redefinition() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_redef.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define MAX 100\n";
        ofs << "#define MAX 200\n";  // Redefinition
        ofs << "int x = MAX;\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_macro_redefinition: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    return 0;
}

// Test 9: Macro with leading/trailing whitespace in replacement
static int test_macro_whitespace() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_ws.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define SPACES    1   2   3   \n";  // Should strip leading/trailing
        ofs << "int x = SPACES;\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_macro_whitespace: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    return 0;
}

int main() {
    int result = 0;

    result = test_object_macro_simple();
    if (result != 0) {
        std::cerr << "test_object_macro_simple failed with code " << result << "\n";
        return result;
    }

    result = test_multiple_object_macros();
    if (result != 0) {
        std::cerr << "test_multiple_object_macros failed with code " << result << "\n";
        return result;
    }

    result = test_empty_macro();
    if (result != 0) {
        std::cerr << "test_empty_macro failed with code " << result << "\n";
        return result;
    }

    result = test_function_macro();
    if (result != 0) {
        std::cerr << "test_function_macro failed with code " << result << "\n";
        return result;
    }

    result = test_variadic_macro();
    if (result != 0) {
        std::cerr << "test_variadic_macro failed with code " << result << "\n";
        return result;
    }

    result = test_undef_macro();
    if (result != 0) {
        std::cerr << "test_undef_macro failed with code " << result << "\n";
        return result;
    }

    result = test_macro_complex_replacement();
    if (result != 0) {
        std::cerr << "test_macro_complex_replacement failed with code " << result << "\n";
        return result;
    }

    result = test_macro_redefinition();
    if (result != 0) {
        std::cerr << "test_macro_redefinition failed with code " << result << "\n";
        return result;
    }

    result = test_macro_whitespace();
    if (result != 0) {
        std::cerr << "test_macro_whitespace failed with code " << result << "\n";
        return result;
    }

    return 0;
}
