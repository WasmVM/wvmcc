#include <iostream>
#include <cassert>
#include <fstream>
#include <cstdio>
#include "pp/ConstExprParser.hpp"
#include "pp/MacroTable.hpp"
#include "pp/Diagnostics.hpp"
#include "pp/Preprocessor.hpp"

// Test 1: Simple integer literal
static int test_integer_literal() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    // Use streaming Preprocessor to produce tokens for the expression
    {
        std::ofstream ofs("temp_constexpr_integer.c");
        ofs << "42\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_integer.c")) {
        std::remove("temp_constexpr_integer.c");
        std::cerr << "test_integer_literal: failed to open input\n";
        return 1;
    }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) {
        if (t->kind == PPTokenKind::Newline) break;
        tokens.push_back(*t);
    }
    std::remove("temp_constexpr_integer.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 42) {
        std::cerr << "test_integer_literal: expected 42\n";
        return 1;
    }
    return 0;
}

// Test 2: Arithmetic expression
static int test_arithmetic() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_arith.c");
        ofs << "2 + 3 * 4\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_arith.c")) { std::remove("temp_constexpr_arith.c"); std::cerr << "test_arithmetic: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_arith.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 14) {
        std::cerr << "test_arithmetic: expected 14, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 3: Parenthesized expression
static int test_parentheses() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_paren.c");
        ofs << "(2 + 3) * 4\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_paren.c")) { std::remove("temp_constexpr_paren.c"); std::cerr << "test_parentheses: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_paren.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 20) {
        std::cerr << "test_parentheses: expected 20, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 4: Logical operators
static int test_logical() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_logical.c");
        ofs << "1 && 0 || 1\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_logical.c")) { std::remove("temp_constexpr_logical.c"); std::cerr << "test_logical: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_logical.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 1) {
        std::cerr << "test_logical: expected 1, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 5: Comparison operators
static int test_comparison() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_comp.c");
        ofs << "3 < 5\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_comp.c")) { std::remove("temp_constexpr_comp.c"); std::cerr << "test_comparison: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_comp.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 1) {
        std::cerr << "test_comparison: expected 1, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 6: Ternary operator
static int test_ternary() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_ternary.c");
        ofs << "1 ? 10 : 20\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_ternary.c")) { std::remove("temp_constexpr_ternary.c"); std::cerr << "test_ternary: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_ternary.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 10) {
        std::cerr << "test_ternary: expected 10, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 7: Unary operators
static int test_unary() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_unary.c");
        ofs << "!0\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_unary.c")) { std::remove("temp_constexpr_unary.c"); std::cerr << "test_unary: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_unary.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 1) {
        std::cerr << "test_unary: expected 1, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 8: defined() operator with defined macro
static int test_defined_true() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    macroTable.defineObjectMacro("MYVAR", {});
    
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_defined_true.c");
        ofs << "defined(MYVAR)\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_defined_true.c")) { std::remove("temp_constexpr_defined_true.c"); std::cerr << "test_defined_true: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_defined_true.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 1) {
        std::cerr << "test_defined_true: expected 1, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 9: defined() operator with undefined macro
static int test_defined_false() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_defined_false.c");
        ofs << "defined(UNDEFINED)\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_defined_false.c")) { std::remove("temp_constexpr_defined_false.c"); std::cerr << "test_defined_false: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_defined_false.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 0) {
        std::cerr << "test_defined_false: expected 0, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 10: Division by zero error
static int test_division_by_zero() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_div0.c");
        ofs << "10 / 0\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_div0.c")) { std::remove("temp_constexpr_div0.c"); std::cerr << "test_division_by_zero: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_div0.c");
    
    auto result = parser.evaluate(tokens);
    if (result) {
        std::cerr << "test_division_by_zero: expected nullopt (error), got " << *result << "\n";
        return 1;
    }
    if (diagnostics.empty()) {
        std::cerr << "test_division_by_zero: expected diagnostic for division by zero\n";
        return 2;
    }
    return 0;
}

// Test 11: Modulo by zero error
static int test_modulo_by_zero() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_mod0.c");
        ofs << "10 % 0\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_mod0.c")) { std::remove("temp_constexpr_mod0.c"); std::cerr << "test_modulo_by_zero: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_mod0.c");
    
    auto result = parser.evaluate(tokens);
    if (result) {
        std::cerr << "test_modulo_by_zero: expected nullopt (error), got " << *result << "\n";
        return 1;
    }
    if (diagnostics.empty()) {
        std::cerr << "test_modulo_by_zero: expected diagnostic for modulo by zero\n";
        return 2;
    }
    return 0;
}

// Test 12: Trailing tokens error
static int test_trailing_tokens() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_trailing.c");
        ofs << "42 garbage\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_trailing.c")) { std::remove("temp_constexpr_trailing.c"); std::cerr << "test_trailing_tokens: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_trailing.c");
    
    auto result = parser.evaluate(tokens);
    if (result) {
        std::cerr << "test_trailing_tokens: expected nullopt (error), got " << *result << "\n";
        return 1;
    }
    if (diagnostics.empty()) {
        std::cerr << "test_trailing_tokens: expected diagnostic for trailing tokens\n";
        return 2;
    }
    return 0;
}

// Test 13: Empty expression error
static int test_empty_expression() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    std::vector<PPToken> tokens; // empty
    
    auto result = parser.evaluate(tokens);
    if (result) {
        std::cerr << "test_empty_expression: expected nullopt (error), got " << *result << "\n";
        return 1;
    }
    if (diagnostics.empty()) {
        std::cerr << "test_empty_expression: expected diagnostic for empty expression\n";
        return 2;
    }
    return 0;
}

// Test 14: Bitwise operators
static int test_bitwise() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_bitwise.c");
        ofs << "5 & 3\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_bitwise.c")) { std::remove("temp_constexpr_bitwise.c"); std::cerr << "test_bitwise: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_bitwise.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 1) {
        std::cerr << "test_bitwise: expected 1, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

// Test 15: Shift operators
static int test_shift() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    {
        std::ofstream ofs("temp_constexpr_shift.c");
        ofs << "1 << 3\n";
    }
    Preprocessor pp;
    if (!pp.open("temp_constexpr_shift.c")) { std::remove("temp_constexpr_shift.c"); std::cerr << "test_shift: failed to open input\n"; return 1; }
    std::vector<PPToken> tokens;
    while (auto t = pp.next()) { if (t->kind == PPTokenKind::Newline) break; tokens.push_back(*t); }
    std::remove("temp_constexpr_shift.c");
    
    auto result = parser.evaluate(tokens);
    if (!result || *result != 8) {
        std::cerr << "test_shift: expected 8, got " << (result ? std::to_string(*result) : "nullopt") << "\n";
        return 1;
    }
    return 0;
}

int main() {
    int result;

    result = test_integer_literal();
    if (result != 0) {
        std::cerr << "test_integer_literal failed with code " << result << "\n";
        return result;
    }

    result = test_arithmetic();
    if (result != 0) {
        std::cerr << "test_arithmetic failed with code " << result << "\n";
        return result;
    }

    result = test_parentheses();
    if (result != 0) {
        std::cerr << "test_parentheses failed with code " << result << "\n";
        return result;
    }

    result = test_logical();
    if (result != 0) {
        std::cerr << "test_logical failed with code " << result << "\n";
        return result;
    }

    result = test_comparison();
    if (result != 0) {
        std::cerr << "test_comparison failed with code " << result << "\n";
        return result;
    }

    result = test_ternary();
    if (result != 0) {
        std::cerr << "test_ternary failed with code " << result << "\n";
        return result;
    }

    result = test_unary();
    if (result != 0) {
        std::cerr << "test_unary failed with code " << result << "\n";
        return result;
    }

    result = test_defined_true();
    if (result != 0) {
        std::cerr << "test_defined_true failed with code " << result << "\n";
        return result;
    }

    result = test_defined_false();
    if (result != 0) {
        std::cerr << "test_defined_false failed with code " << result << "\n";
        return result;
    }

    result = test_division_by_zero();
    if (result != 0) {
        std::cerr << "test_division_by_zero failed with code " << result << "\n";
        return result;
    }

    result = test_modulo_by_zero();
    if (result != 0) {
        std::cerr << "test_modulo_by_zero failed with code " << result << "\n";
        return result;
    }

    result = test_trailing_tokens();
    if (result != 0) {
        std::cerr << "test_trailing_tokens failed with code " << result << "\n";
        return result;
    }

    result = test_empty_expression();
    if (result != 0) {
        std::cerr << "test_empty_expression failed with code " << result << "\n";
        return result;
    }

    result = test_bitwise();
    if (result != 0) {
        std::cerr << "test_bitwise failed with code " << result << "\n";
        return result;
    }

    result = test_shift();
    if (result != 0) {
        std::cerr << "test_shift failed with code " << result << "\n";
        return result;
    }

    std::cout << "pp_constexpr_test: all cases passed\n";
    return 0;
}
