#pragma once

#include <optional>
#include <vector>
#include <cstdint>
#include "Tokenizer.hpp"
#include "MacroTable.hpp"

#include "../common.hpp"

namespace wvmcc {

// Parses and evaluates preprocessor constant integer expressions (C17 6.6).
class ConstExprParser {
public:
    ConstExprParser(MacroTable& macroTable, std::vector<Diagnostic>& diagnostics);

    // Evaluate an already macro-expanded token sequence as an integer constant expression.
    // Returns std::nullopt on parse or evaluation error and emits diagnostics.
    //
    // Per C17 6.10.1p4, the controlling expression is evaluated with intmax_t /
    // uintmax_t (here 64-bit). The result is returned as an int64_t whose bit
    // pattern is the uintmax_t/intmax_t result; callers only test it against 0
    // for truthiness, so the (potentially unsigned) value round-trips correctly.
    std::optional<int64_t> evaluate(const std::vector<PPToken>& tokens);

private:
    // A preprocessor constant-expression operand. Values are held in a
    // uintmax_t-width slot; `isUnsigned` records whether the operand has
    // unsigned type (uintmax_t) — set for u/U-suffixed constants and for any
    // constant whose value exceeds INTMAX_MAX (6.4.4.1p5), and propagated
    // through the usual arithmetic conversions (6.3.1.8): if either operand of
    // a binary operator is unsigned, the operation is performed unsigned.
    struct Value {
        uint64_t bits = 0;
        bool isUnsigned = false;
        bool nonzero() const { return bits != 0; }
    };

    // Usual arithmetic conversions (6.3.1.8): both operands have
    // intmax_t/uintmax_t rank (64-bit), so the result is unsigned iff either
    // operand is unsigned.
    static bool combineUnsigned(const Value& a, const Value& b) {
        return a.isUnsigned || b.isUnsigned;
    }

    void skipWhitespace(const std::vector<PPToken>& tokens, size_t end, size_t& pos) const;
    std::optional<Value> parseExpr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseTernary(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseLogicalOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseLogicalAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseBitwiseOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseBitwiseXor(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseBitwiseAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseEquality(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseRelational(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseShift(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseAdditive(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseMultiplicative(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parseUnary(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> parsePrimary(const std::vector<PPToken>& tokens, size_t end, size_t& pos);

    // Helper methods for parsePrimary
    std::optional<Value> tryParsePPNumber(const PPToken& tok, size_t& pos);
    std::optional<Value> tryParseCharConst(const PPToken& tok, size_t& pos);
    std::optional<Value> tryParseParenExpr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<Value> tryParseDefined(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    bool parseDefinedMacroName(const std::vector<PPToken>& tokens, size_t end, size_t& pos,
                               const SourceSpan& tokSpan, std::string& macroName);
    bool checkDefinedCloseParen(const std::vector<PPToken>& tokens, size_t end, size_t& pos);

    MacroTable& macroTable;
    std::vector<Diagnostic>& diagnostics;
};

} // namespace wvmcc
