#include "pp/Preprocessor.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

static bool preprocess_collect_tokens(const std::string &srcName,
                                      std::vector<wvmcc::PPToken> &outTokens,
                                      const char *testLabel) {
    using namespace wvmcc;
    Preprocessor pp;
    if (!pp.open(srcName)) {
        std::cerr << testLabel << ": failed to open stream\n";
        std::remove(srcName.c_str());
        return false;
    }
    while (auto t = pp.next()) outTokens.push_back(*t);
    std::remove(srcName.c_str());
    for (const auto &d : pp.getDiagnostics()) {
        if (d.severity == Diagnostic::Severity::Error) {
            std::cerr << testLabel << ": preprocess produced errors\n";
            return false;
        }
    }
    return true;
}

// Test 1: Simple object-like macro
static int test_object_macro_simple() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_simple.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define MAX 100\n";
        ofs << "int x = MAX;\n";
    }

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_object_macro_simple")) return 1;

    bool hasMax = false, has100 = false;
    for (const auto& t : tokens) {
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_multiple_object_macros")) return 1;
    bool found100 = false, found0 = false, found42 = false;
    bool foundMAX = false, foundMIN = false, foundSIZE = false;
    for (const auto &t : tokens) {
        if (t.lexeme == "100") found100 = true;
        if (t.lexeme == "0") found0 = true;
        if (t.lexeme == "42") found42 = true;
        if (t.lexeme == "MAX") foundMAX = true;
        if (t.lexeme == "MIN") foundMIN = true;
        if (t.lexeme == "SIZE") foundSIZE = true;
    }
    if (!found100 || !found0 || !found42) {
        std::cerr << "test_multiple_object_macros: expected expanded values 100,0,42\n";
        return 2;
    }
    if (foundMAX || foundMIN || foundSIZE) {
        std::cerr << "test_multiple_object_macros: macro names should be expanded away\n";
        return 3;
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_empty_macro")) return 1;
    bool foundDEBUG = false, foundVoid = false;
    for (const auto &t : tokens) {
        if (t.lexeme == "DEBUG") foundDEBUG = true;
        if (t.lexeme == "void") foundVoid = true;
    }
    if (foundDEBUG) {
        std::cerr << "test_empty_macro: DEBUG should not appear in output\n";
        return 2;
    }
    if (!foundVoid) {
        std::cerr << "test_empty_macro: expected 'void' in output\n";
        return 3;
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_function_macro")) return 1;
    bool found2 = false, found3 = false, foundADD = false;
    for (const auto &t : tokens) {
        if (t.lexeme == "2") found2 = true;
        if (t.lexeme == "3") found3 = true;
        if (t.lexeme == "ADD") foundADD = true;
    }
    if (!found2 || !found3) {
        std::cerr << "test_function_macro: expected '2' and '3' after expansion\n";
        return 2;
    }
    if (foundADD) {
        std::cerr << "test_function_macro: macro name 'ADD' should not appear after expansion\n";
        return 3;
    }
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_variadic_macro")) return 1;
    bool foundPrintf = false;
    bool foundTest = false;
    bool found42 = false;
    bool foundPRINTF = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "printf") foundPrintf = true;
        if (t.lexeme == "test") foundTest = true;
        if (t.lexeme == "42") found42 = true;
        if (t.lexeme == "PRINTF") foundPRINTF = true;
    }
    if (!foundPrintf || !foundTest || !found42) {
        std::cerr << "test_variadic_macro: expected 'printf', '\"test\"', and '42' in output\n";
        std::cerr << "found: printf=" << foundPrintf << " test=" << foundTest << " 42=" << found42 << "\n";
        return 2;
    }
    if (foundPRINTF) {
        std::cerr << "test_variadic_macro: macro name 'PRINTF' should not appear after expansion\n";
        return 3;
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_stringification")) return 1;
    bool foundStringHello = false;
    for (const auto& t : tokens) {
        if (t.kind == wvmcc::PPTokenKind::StringLiteral && t.lexeme == "\"hello\"") {
            foundStringHello = true;
            break;
        }
    }
    if (!foundStringHello) {
        std::cerr << "test_stringification: expected string literal \"hello\" in output\n";
        std::cerr << "tokens:\n";
        for (const auto& t : tokens) {
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_stringification_special")) return 1;
    bool found1Plus2 = false;
    bool foundQuoted = false;
    for (const auto& t : tokens) {
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_token_pasting")) return 1;
    bool foundFoobar = false;
    for (const auto& t : tokens) {
        if (t.kind == wvmcc::PPTokenKind::Identifier && t.lexeme == "foobar") {
            foundFoobar = true;
            break;
        }
    }
    if (!foundFoobar) {
        std::cerr << "test_token_pasting: expected identifier 'foobar' in output\n";
        std::cerr << "tokens:\n";
        for (const auto& t : tokens) {
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_token_pasting_numbers")) return 1;
    bool foundV1_2 = false;
    for (const auto& t : tokens) {
        if (t.kind == wvmcc::PPTokenKind::Identifier && t.lexeme == "v1_2") {
            foundV1_2 = true;
            break;
        }
    }
    if (!foundV1_2) {
        std::cerr << "test_token_pasting_numbers: expected identifier 'v1_2' in output\n";
        std::cerr << "tokens:\n";
        for (const auto& t : tokens) {
            std::cerr << "  " << t.lexeme << "\n";
        }
        return 2;
    }
    
    return 0;
}

// Test 11: Predefined macros
static int test_predefined_macros() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_predefined.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "const char* file = __FILE__;\n";
        ofs << "const char* date = __DATE__;\n";
        ofs << "const char* time = __TIME__;\n";
        ofs << "int stdc = __STDC__;\n";
        ofs << "int version = __STDC_VERSION__;\n";
    }

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_predefined_macros")) return 1;
    bool foundFile = false, foundDate = false, foundTime = false;
    bool foundStdc = false, foundVersion = false;
    for (const auto& t : tokens) {
        if (t.kind == wvmcc::PPTokenKind::StringLiteral && t.lexeme.find("temp_macro_predefined.c") != std::string::npos) {
            foundFile = true;
        }
        if (t.kind == wvmcc::PPTokenKind::StringLiteral && t.lexeme.find(" 20") != std::string::npos) {
            // DATE contains year like 2025
            foundDate = true;
        }
        if (t.kind == wvmcc::PPTokenKind::StringLiteral && t.lexeme.find(":") != std::string::npos) {
            // TIME contains colons HH:MM:SS
            foundTime = true;
        }
        if (t.kind == wvmcc::PPTokenKind::PPNumber && t.lexeme == "1") {
            foundStdc = true;
        }
        if (t.kind == wvmcc::PPTokenKind::PPNumber && t.lexeme == "201710L") {
            foundVersion = true;
        }
    }
    if (!foundFile || !foundDate || !foundTime || !foundStdc || !foundVersion) {
        std::cerr << "test_predefined_macros: missing predefined macros\n";
        std::cerr << "found: file=" << foundFile << " date=" << foundDate << " time=" << foundTime
                  << " stdc=" << foundStdc << " version=" << foundVersion << "\n";
        return 2;
    }
    
    return 0;
}

// Test 12: Undefine macro
static int test_undef_macro() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_undef.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define MAX 100\n";
        ofs << "#undef MAX\n";
        ofs << "int x = MAX;\n";  // Should be treated as identifier, not macro
    }

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_undef_macro")) return 1;
    bool foundMAX = false, found100 = false;
    for (const auto &t : tokens) {
        if (t.lexeme == "MAX") foundMAX = true;
        if (t.lexeme == "100") found100 = true;
    }
    if (!foundMAX) {
        std::cerr << "test_undef_macro: expected 'MAX' identifier after undef\n";
        return 2;
    }
    if (found100) {
        std::cerr << "test_undef_macro: unexpected '100' after undef\n";
        return 3;
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_variadic_empty")) return 1;
    bool foundLogFunc = false;
    for (const auto& t : tokens) {
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

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_variadic_multiple")) return 1;
    bool foundAdd = false;
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& t : tokens) {
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

// Test 13: Object-like macro with complex replacement
static int test_macro_complex_replacement() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_complex.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define SWAP(a, b) { int temp = a; a = b; b = temp; }\n";
        ofs << "void swap_ints(int *x, int *y) SWAP(*x, *y)\n";
    }

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_macro_complex_replacement")) return 1;
    bool foundSWAP = false, foundTemp = false, foundBrace = false;
    for (const auto &t : tokens) {
        if (t.lexeme == "SWAP") foundSWAP = true;
        if (t.lexeme == "temp") foundTemp = true;
        if (t.lexeme == "{") foundBrace = true;
    }
    if (foundSWAP) {
        std::cerr << "test_macro_complex_replacement: macro name 'SWAP' should not appear after expansion\n";
        return 2;
    }
    if (!foundTemp && !foundBrace) {
        std::cerr << "test_macro_complex_replacement: expected expansion tokens (e.g. 'temp' or '{')\n";
        return 3;
    }
    return 0;
}

// Test 14: Macro redefinition (should succeed - replaces previous definition)
static int test_macro_redefinition() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_redef.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define MAX 100\n";
        ofs << "#define MAX 200\n";  // Redefinition
        ofs << "int x = MAX;\n";
    }

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_macro_redefinition")) return 1;
    bool foundMax = false;
    for (const auto& t : tokens) {
        if (t.lexeme == "200") foundMax = true;
    }
    if (!foundMax) {
        std::cerr << "test_macro_redefinition: expected '200' in output\n";
        return 2;
    }
    return 0;
}

// Test 15: Macro with leading/trailing whitespace in replacement
static int test_macro_whitespace() {
    using namespace wvmcc;
    const std::string srcName = "temp_macro_ws.c";
    
    {
        std::ofstream ofs(srcName);
        ofs << "#define SPACES    1   2   3   \n";  // Should strip leading/trailing
        ofs << "int x = SPACES;\n";
    }

    std::vector<wvmcc::PPToken> tokens;
    if (!preprocess_collect_tokens(srcName, tokens, "test_macro_whitespace")) return 1;
    bool found1 = false, found2 = false, found3 = false, foundSPACES = false;
    for (const auto &t : tokens) {
        if (t.lexeme == "1") found1 = true;
        if (t.lexeme == "2") found2 = true;
        if (t.lexeme == "3") found3 = true;
        if (t.lexeme == "SPACES") foundSPACES = true;
    }
    if (!found1 || !found2 || !found3) {
        std::cerr << "test_macro_whitespace: expected '1', '2', '3' after expansion\n";
        return 2;
    }
    if (foundSPACES) {
        std::cerr << "test_macro_whitespace: macro name 'SPACES' should not appear after expansion\n";
        return 3;
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

    result = test_predefined_macros();
    if (result != 0) {
        std::cerr << "test_predefined_macros failed with code " << result << "\n";
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
