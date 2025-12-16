#pragma once

#include <optional>
#include <vector>
#include <cstdint>
#include "Tokenizer.hpp"
#include "MacroTable.hpp"

#include "Diagnostics.hpp"

namespace wvmcc {

// Parses and evaluates preprocessor constant integer expressions (C17 6.6).
class ConstExprParser {
public:
    ConstExprParser(MacroTable& macroTable, std::vector<Diagnostic>& diagnostics);

    // Evaluate an already macro-expanded token sequence as an integer constant expression.
    // Returns std::nullopt on parse or evaluation error and emits diagnostics.
    std::optional<int64_t> evaluate(const std::vector<PPToken>& tokens);

private:
    void skipWhitespace(const std::vector<PPToken>& tokens, size_t end, size_t& pos) const;
    std::optional<int64_t> parseExpr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseTernary(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseLogicalOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseLogicalAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseBitwiseOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseBitwiseXor(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseBitwiseAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseEquality(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseRelational(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseShift(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseAdditive(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseMultiplicative(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parseUnary(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> parsePrimary(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    
    // Helper methods for parsePrimary
    std::optional<int64_t> tryParsePPNumber(const PPToken& tok, size_t& pos);
    std::optional<int64_t> tryParseCharConst(const PPToken& tok, size_t& pos);
    std::optional<int64_t> tryParseParenExpr(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    std::optional<int64_t> tryParseDefined(const std::vector<PPToken>& tokens, size_t end, size_t& pos);
    bool parseDefinedMacroName(const std::vector<PPToken>& tokens, size_t end, size_t& pos,
                               const SourceSpan& tokSpan, std::string& macroName);
    bool checkDefinedCloseParen(const std::vector<PPToken>& tokens, size_t end, size_t& pos);

    MacroTable& macroTable;
    std::vector<Diagnostic>& diagnostics;
};

} // namespace wvmcc
