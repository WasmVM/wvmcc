#include <iostream>
#include <cassert>
#include "../src/pp/ConstExprParser.hpp"
#include "../src/pp/MacroTable.hpp"
#include "../src/pp/Diagnostics.hpp"

// Test 1: Simple integer literal
static int test_integer_literal() {
    using namespace wvmcc;
    
    MacroTable macroTable;
    std::vector<Diagnostic> diagnostics;
    ConstExprParser parser(macroTable, diagnostics);
    
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{
        .kind = PPTokenKind::PPNumber,
        .lexeme = "42",
        .span = SourceSpan{SourcePos{0,1,1,0}, SourcePos{0,1,3,2}}
    });
    
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
    
    // 2 + 3 * 4
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "2", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "+", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "3", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "*", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "4", .span = {}});
    
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
    
    // (2 + 3) * 4
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "(", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "2", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "+", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "3", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = ")", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "*", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "4", .span = {}});
    
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
    
    // 1 && 0 || 1
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "1", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "&&", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "0", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "||", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "1", .span = {}});
    
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
    
    // 3 < 5
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "3", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "<", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "5", .span = {}});
    
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
    
    // 1 ? 10 : 20
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "1", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "?", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "10", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = ":", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "20", .span = {}});
    
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
    
    // !0
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "!", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "0", .span = {}});
    
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
    
    // defined(MYVAR)
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::Identifier, .lexeme = "defined", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "(", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Identifier, .lexeme = "MYVAR", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = ")", .span = {}});
    
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
    
    // defined(UNDEFINED)
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::Identifier, .lexeme = "defined", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "(", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Identifier, .lexeme = "UNDEFINED", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = ")", .span = {}});
    
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
    
    // 10 / 0
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "10", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "/", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "0", .span = {}});
    
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
    
    // 10 % 0
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "10", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "%", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "0", .span = {}});
    
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
    
    // 42 garbage
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "42", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Identifier, .lexeme = "garbage", .span = {}});
    
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
    
    // 5 & 3
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "5", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "&", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "3", .span = {}});
    
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
    
    // 1 << 3
    std::vector<PPToken> tokens;
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "1", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::Punctuator, .lexeme = "<<", .span = {}});
    tokens.push_back(PPToken{.kind = PPTokenKind::PPNumber, .lexeme = "3", .span = {}});
    
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
