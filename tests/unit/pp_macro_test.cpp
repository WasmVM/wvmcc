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
    
    // Verify __VA_ARGS__ was replaced with 42
    bool foundPrintf = false;
    bool foundTest = false;
    bool found42 = false;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "printf") foundPrintf = true;
        if (t.lexeme == "test") foundTest = true;
        if (t.lexeme == "42") found42 = true;
    }
    
    if (!foundPrintf || !foundTest || !found42) {
        std::cerr << "test_variadic_macro: expected 'printf', '\"test\"', and '42' in output\n";
        std::cerr << "found: printf=" << foundPrintf << " test=" << foundTest << " 42=" << found42 << "\n";
        return 2;
    }
    
    return 0;
}

// Test 6: Stringification operator (#)
static int test_stringification() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_stringify.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define STR(x) #x\n";
        ofs << "const char* s = STR(hello);\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_stringification: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    // Verify stringification: STR(hello) should become "hello"
    bool foundStringHello = false;
    for (const auto& t : res.tokens) {
        if (t.kind == wvmcc::PPTokenKind::StringLiteral && t.lexeme == "\"hello\"") {
            foundStringHello = true;
            break;
        }
    }
    
    if (!foundStringHello) {
        std::cerr << "test_stringification: expected string literal \"hello\" in output\n";
        std::cerr << "tokens:\n";
        for (const auto& t : res.tokens) {
            std::cerr << "  " << t.lexeme << "\n";
        }
        return 2;
    }
    
    return 0;
}

// Test 7: Stringification with special characters
static int test_stringification_special() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_stringify_special.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define STR(x) #x\n";
        ofs << "const char* s1 = STR(1 + 2);\n";
        ofs << "const char* s2 = STR(\"quoted\");\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_stringification_special: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    // Verify stringification of expressions: STR(1 + 2) -> "1 + 2"
    // and STR("quoted") -> "\"quoted\"" (with escaped quotes)
    bool found1Plus2 = false;
    bool foundQuoted = false;
    for (const auto& t : res.tokens) {
        if (t.kind == wvmcc::PPTokenKind::StringLiteral) {
            if (t.lexeme.find("1") != std::string::npos && t.lexeme.find("+") != std::string::npos) {
                found1Plus2 = true;
            }
            if (t.lexeme.find("\\\"") != std::string::npos) {
                foundQuoted = true;
            }
        }
    }
    
    if (!found1Plus2 || !foundQuoted) {
        std::cerr << "test_stringification_special: expected stringified expressions\n";
        std::cerr << "found: 1Plus2=" << found1Plus2 << " quoted=" << foundQuoted << "\n";
        return 2;
    }
    
    return 0;
}

// Test 8: Token pasting operator (##)
static int test_token_pasting() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_paste.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define CONCAT(a, b) a ## b\n";
        ofs << "int foobar = CONCAT(foo, bar);\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_token_pasting: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    // Verify token pasting: CONCAT(foo, bar) should become foobar
    bool foundFoobar = false;
    for (const auto& t : res.tokens) {
        if (t.kind == wvmcc::PPTokenKind::Identifier && t.lexeme == "foobar") {
            foundFoobar = true;
            break;
        }
    }
    
    if (!foundFoobar) {
        std::cerr << "test_token_pasting: expected identifier 'foobar' in output\n";
        std::cerr << "tokens:\n";
        for (const auto& t : res.tokens) {
            std::cerr << "  " << t.lexeme << "\n";
        }
        return 2;
    }
    
    return 0;
}

// Test 9: Token pasting with numbers
static int test_token_pasting_numbers() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_paste_numbers.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define VERSION(major, minor) v ## major ## _ ## minor\n";
        ofs << "const char* ver = VERSION(1, 2);\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_token_pasting_numbers: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    // Verify: VERSION(1, 2) should become v1_2
    bool foundV1_2 = false;
    for (const auto& t : res.tokens) {
        if (t.kind == wvmcc::PPTokenKind::Identifier && t.lexeme == "v1_2") {
            foundV1_2 = true;
            break;
        }
    }
    
    if (!foundV1_2) {
        std::cerr << "test_token_pasting_numbers: expected identifier 'v1_2' in output\n";
        std::cerr << "tokens:\n";
        for (const auto& t : res.tokens) {
            std::cerr << "  " << t.lexeme << "\n";
        }
        return 2;
    }
    
    return 0;
}

// Test 10: Undefine macro
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

// Test 7a: Variadic macro with empty variadic args
static int test_variadic_empty() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_variadic_empty.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define LOG(msg, ...) log_func(msg, __VA_ARGS__)\n";
        ofs << "void test() { LOG(\"only message\"); }\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_variadic_empty: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    // When variadic is empty, __VA_ARGS__ expands to nothing
    // Result: log_func("only message", )
    bool foundLogFunc = false;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "log_func") foundLogFunc = true;
    }
    
    if (!foundLogFunc) {
        std::cerr << "test_variadic_empty: expected 'log_func' in output\n";
        return 2;
    }
    
    return 0;
}

// Test 7b: Variadic macro with multiple arguments
static int test_variadic_multiple() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_variadic_multiple.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define CALL(fn, ...) fn(__VA_ARGS__)\n";
        ofs << "int result = CALL(add, 1, 2, 3);\n";
    }

    Preprocessor pp;
    auto res = pp.run(srcName);
    std::remove(srcName.c_str());

    if (!res.success) {
        std::cerr << "test_variadic_multiple: preprocess failed: " << res.errorMsg << "\n";
        return 1;
    }
    
    // Should expand to: add(1, 2, 3)
    bool foundAdd = false;
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& t : res.tokens) {
        if (t.lexeme == "add") foundAdd = true;
        if (t.lexeme == "1") found1 = true;
        if (t.lexeme == "2") found2 = true;
        if (t.lexeme == "3") found3 = true;
    }
    
    if (!foundAdd || !found1 || !found2 || !found3) {
        std::cerr << "test_variadic_multiple: expected 'add', '1', '2', '3' in output\n";
        std::cerr << "found: add=" << foundAdd << " 1=" << found1 << " 2=" << found2 << " 3=" << found3 << "\n";
        return 2;
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

    result = test_stringification();
    if (result != 0) {
        std::cerr << "test_stringification failed with code " << result << "\n";
        return result;
    }

    result = test_stringification_special();
    if (result != 0) {
        std::cerr << "test_stringification_special failed with code " << result << "\n";
        return result;
    }

    result = test_token_pasting();
    if (result != 0) {
        std::cerr << "test_token_pasting failed with code " << result << "\n";
        return result;
    }

    result = test_token_pasting_numbers();
    if (result != 0) {
        std::cerr << "test_token_pasting_numbers failed with code " << result << "\n";
        return result;
    }

    result = test_undef_macro();
    if (result != 0) {
        std::cerr << "test_undef_macro failed with code " << result << "\n";
        return result;
    }

    result = test_variadic_empty();
    if (result != 0) {
        std::cerr << "test_variadic_empty failed with code " << result << "\n";
        return result;
    }

    result = test_variadic_multiple();
    if (result != 0) {
        std::cerr << "test_variadic_multiple failed with code " << result << "\n";
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
