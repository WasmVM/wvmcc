#include <cstdint>
#include "ConstExprParser.hpp"

#include <string>

namespace wvmcc {

ConstExprParser::ConstExprParser(MacroTable& macroTableRef, std::vector<Diagnostic>& diagnosticsRef)
    : macroTable(macroTableRef), diagnostics(diagnosticsRef) {}

std::optional<int64_t> ConstExprParser::evaluate(const std::vector<PPToken>& tokens) {
    // Skip leading/trailing whitespace
    size_t i = 0;
    while (i < tokens.size() && tokens[i].kind == PPTokenKind::Whitespace) i++;
    size_t end = tokens.size();
    while (end > i && tokens[end - 1].kind == PPTokenKind::Whitespace) end--;

    if (i >= end) {
        diagnostics.push_back(Diagnostic{
            .message = "empty constant expression in #if/#elif",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return std::nullopt;
    }

    size_t pos = i;
    auto result = parseExpr(tokens, end, pos);
    if (result && pos < end) {
        diagnostics.push_back(Diagnostic{
            .message = "unexpected tokens after constant expression",
            .severity = Diagnostic::Severity::Error,
            .span = tokens[pos].span
        });
        return std::nullopt;
    }
    if (!result) return std::nullopt;
    // Return the raw bit pattern. Callers only test against 0 for truthiness,
    // so an unsigned result above INT64_MAX still round-trips as nonzero.
    return static_cast<int64_t>(result->bits);
}

void ConstExprParser::skipWhitespace(const std::vector<PPToken>& tokens, size_t end, size_t& pos) const {
    while (pos < end && tokens[pos].kind == PPTokenKind::Whitespace) pos++;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseExpr(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    return parseTernary(tokens, end, pos);
}

std::optional<ConstExprParser::Value> ConstExprParser::parseTernary(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    skipWhitespace(tokens, end, pos);
    auto val = parseLogicalOr(tokens, end, pos);
    if (!val) return std::nullopt;

    skipWhitespace(tokens, end, pos);
    if (pos < end && tokens[pos].kind == PPTokenKind::Punctuator && tokens[pos].lexeme == "?") {
        pos++;
        auto trueVal = parseExpr(tokens, end, pos);
        if (!trueVal) return std::nullopt;
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != ":") {
            diagnostics.push_back(Diagnostic{
                .message = "expected ':' in ternary operator",
                .severity = Diagnostic::Severity::Error,
                .span = (pos < end) ? std::optional<SourceSpan>(tokens[pos].span) : std::optional<SourceSpan>()
            });
            return std::nullopt;
        }
        pos++;
        auto falseVal = parseTernary(tokens, end, pos);
        if (!falseVal) return std::nullopt;
        // The result type follows the usual arithmetic conversions of the two
        // branches (6.5.15p5); only the selected branch's value is used.
        Value result = val->nonzero() ? *trueVal : *falseVal;
        result.isUnsigned = combineUnsigned(*trueVal, *falseVal);
        return result;
    }
    return val;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseLogicalOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseLogicalAnd(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "||") break;
        pos++;
        auto right = parseLogicalAnd(tokens, end, pos);
        if (!right) return std::nullopt;
        // Result of || is int (signed) with value 0 or 1.
        left = Value{ (left->nonzero() || right->nonzero()) ? 1u : 0u, false };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseLogicalAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseBitwiseOr(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "&&") break;
        pos++;
        auto right = parseBitwiseOr(tokens, end, pos);
        if (!right) return std::nullopt;
        left = Value{ (left->nonzero() && right->nonzero()) ? 1u : 0u, false };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseBitwiseOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseBitwiseXor(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "|" ||
            (pos + 1 < end && tokens[pos + 1].lexeme == "|")) break;
        pos++;
        auto right = parseBitwiseXor(tokens, end, pos);
        if (!right) return std::nullopt;
        left = Value{ left->bits | right->bits, combineUnsigned(*left, *right) };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseBitwiseXor(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseBitwiseAnd(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "^") break;
        pos++;
        auto right = parseBitwiseAnd(tokens, end, pos);
        if (!right) return std::nullopt;
        left = Value{ left->bits ^ right->bits, combineUnsigned(*left, *right) };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseBitwiseAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseEquality(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "&" ||
            (pos + 1 < end && tokens[pos + 1].lexeme == "&")) break;
        pos++;
        auto right = parseEquality(tokens, end, pos);
        if (!right) return std::nullopt;
        left = Value{ left->bits & right->bits, combineUnsigned(*left, *right) };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseEquality(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseRelational(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator ||
            (tokens[pos].lexeme != "==" && tokens[pos].lexeme != "!=")) break;
        std::string op = tokens[pos].lexeme;
        pos++;
        auto right = parseRelational(tokens, end, pos);
        if (!right) return std::nullopt;
        // == / != compare equal bit patterns regardless of signedness; result is int.
        bool eq = left->bits == right->bits;
        left = Value{ ((op == "==") ? eq : !eq) ? 1u : 0u, false };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseRelational(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseShift(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator ||
            (tokens[pos].lexeme != "<" && tokens[pos].lexeme != ">" &&
             tokens[pos].lexeme != "<=" && tokens[pos].lexeme != ">=")) break;
        std::string op = tokens[pos].lexeme;
        pos++;
        auto right = parseShift(tokens, end, pos);
        if (!right) return std::nullopt;
        bool res;
        if (combineUnsigned(*left, *right)) {
            uint64_t l = left->bits, r = right->bits;
            if (op == "<") res = l < r;
            else if (op == ">") res = l > r;
            else if (op == "<=") res = l <= r;
            else res = l >= r;
        } else {
            int64_t l = static_cast<int64_t>(left->bits), r = static_cast<int64_t>(right->bits);
            if (op == "<") res = l < r;
            else if (op == ">") res = l > r;
            else if (op == "<=") res = l <= r;
            else res = l >= r;
        }
        left = Value{ res ? 1u : 0u, false };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseShift(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseAdditive(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator ||
            (tokens[pos].lexeme != "<<" && tokens[pos].lexeme != ">>")) break;
        std::string op = tokens[pos].lexeme;
        pos++;
        auto right = parseAdditive(tokens, end, pos);
        if (!right) return std::nullopt;
        // The type of a shift result is that of the (promoted) left operand;
        // the right operand's signedness does not participate (6.5.7p3).
        unsigned shift = static_cast<unsigned>(right->bits & 63u);
        uint64_t bits;
        if (op == "<<") {
            bits = left->bits << shift;
        } else {
            // >> on a signed left operand is arithmetic; on unsigned, logical.
            if (left->isUnsigned) {
                bits = left->bits >> shift;
            } else {
                bits = static_cast<uint64_t>(static_cast<int64_t>(left->bits) >> shift);
            }
        }
        left = Value{ bits, left->isUnsigned };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseAdditive(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseMultiplicative(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator ||
            (tokens[pos].lexeme != "+" && tokens[pos].lexeme != "-")) break;
        std::string op = tokens[pos].lexeme;
        pos++;
        auto right = parseMultiplicative(tokens, end, pos);
        if (!right) return std::nullopt;
        // Wrapping (modular) arithmetic on the 64-bit slot is correct for both
        // signed (two's complement) and unsigned operands.
        uint64_t bits = (op == "+") ? (left->bits + right->bits) : (left->bits - right->bits);
        left = Value{ bits, combineUnsigned(*left, *right) };
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseMultiplicative(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseUnary(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator ||
            (tokens[pos].lexeme != "*" && tokens[pos].lexeme != "/" && tokens[pos].lexeme != "%")) break;
        std::string op = tokens[pos].lexeme;
        pos++;
        auto right = parseUnary(tokens, end, pos);
        if (!right) return std::nullopt;
        bool isUns = combineUnsigned(*left, *right);
        if (op == "*") {
            left = Value{ left->bits * right->bits, isUns };
        } else if (op == "/") {
            if (right->bits == 0) {
                diagnostics.push_back(Diagnostic{
                    .message = "division by zero in constant expression",
                    .severity = Diagnostic::Severity::Error,
                    .span = tokens[pos - 1].span
                });
                return std::nullopt;
            }
            uint64_t bits = isUns
                ? (left->bits / right->bits)
                : static_cast<uint64_t>(static_cast<int64_t>(left->bits) / static_cast<int64_t>(right->bits));
            left = Value{ bits, isUns };
        } else { // %
            if (right->bits == 0) {
                diagnostics.push_back(Diagnostic{
                    .message = "modulo by zero in constant expression",
                    .severity = Diagnostic::Severity::Error,
                    .span = tokens[pos - 1].span
                });
                return std::nullopt;
            }
            uint64_t bits = isUns
                ? (left->bits % right->bits)
                : static_cast<uint64_t>(static_cast<int64_t>(left->bits) % static_cast<int64_t>(right->bits));
            left = Value{ bits, isUns };
        }
    }
    return left;
}

std::optional<ConstExprParser::Value> ConstExprParser::parseUnary(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    skipWhitespace(tokens, end, pos);
    if (pos < end && tokens[pos].kind == PPTokenKind::Punctuator &&
        (tokens[pos].lexeme == "!" || tokens[pos].lexeme == "~" ||
         tokens[pos].lexeme == "+" || tokens[pos].lexeme == "-")) {
        std::string op = tokens[pos].lexeme;
        pos++;
        auto val = parseUnary(tokens, end, pos);
        if (!val) return std::nullopt;
        if (op == "!") return Value{ val->nonzero() ? 0u : 1u, false };
        else if (op == "~") return Value{ ~val->bits, val->isUnsigned };
        else if (op == "-") return Value{ static_cast<uint64_t>(0) - val->bits, val->isUnsigned };
        else return *val; // unary +
    }
    return parsePrimary(tokens, end, pos);
}

std::optional<ConstExprParser::Value> ConstExprParser::tryParsePPNumber(const PPToken& tok, size_t& pos) {
    if (tok.kind != PPTokenKind::PPNumber) return std::nullopt;

    const std::string& lex = tok.lexeme;

    auto invalidConst = [&]() -> std::optional<Value> {
        diagnostics.push_back(Diagnostic{
            .message = std::string("invalid integer constant: ") + lex,
            .severity = Diagnostic::Severity::Error,
            .span = tok.span
        });
        return std::nullopt;
    };

    // Reject floating-point constants: a pp-number containing '.', or a
    // decimal exponent ('e'/'E') / hex exponent ('p'/'P'), is not an integer
    // constant. In a hex constant 'e'/'E' are valid digits, so only 'p'/'P'
    // marks a (hex) float there.
    bool isHex = lex.size() >= 2 && lex[0] == '0' && (lex[1] == 'x' || lex[1] == 'X');
    for (char c : lex) {
        if (c == '.' || c == 'p' || c == 'P') {
            diagnostics.push_back(Diagnostic{
                .message = std::string("floating constant in preprocessor constant expression: ") + lex,
                .severity = Diagnostic::Severity::Error,
                .span = tok.span
            });
            return std::nullopt;
        }
        if (!isHex && (c == 'e' || c == 'E')) {
            diagnostics.push_back(Diagnostic{
                .message = std::string("floating constant in preprocessor constant expression: ") + lex,
                .severity = Diagnostic::Severity::Error,
                .span = tok.span
            });
            return std::nullopt;
        }
    }

    // Split off the integer-suffix (any mix of u/U and l/L). Detect 'u'/'U'
    // for unsignedness; the l/L count doesn't matter at uintmax_t width.
    size_t sufStart = lex.size();
    bool sawUnsignedSuffix = false;
    while (sufStart > 0) {
        char c = lex[sufStart - 1];
        if (c == 'u' || c == 'U') { sawUnsignedSuffix = true; sufStart--; }
        else if (c == 'l' || c == 'L') { sufStart--; }
        else break;
    }
    std::string digits = lex.substr(0, sufStart);
    if (digits.empty()) return invalidConst();

    // Parse as uintmax_t (base 0 handles 0x / 0 / decimal). std::stoull accepts
    // the full unsigned 64-bit range, including UINT64_MAX.
    uint64_t value = 0;
    try {
        size_t consumed = 0;
        value = std::stoull(digits, &consumed, 0);
        if (consumed != digits.size()) return invalidConst();
    } catch (...) {
        return invalidConst();
    }

    // 6.4.4.1p5: an unsuffixed (or l/L-only) constant that does not fit in
    // intmax_t has type uintmax_t. A u/U-suffixed constant is always unsigned.
    // At uintmax_t width, "exceeds INTMAX_MAX" means the high bit is set.
    bool isUnsigned = sawUnsignedSuffix || (value > static_cast<uint64_t>(INT64_MAX));
    pos++;
    return Value{ value, isUnsigned };
}

std::optional<ConstExprParser::Value> ConstExprParser::tryParseCharConst(const PPToken& tok, size_t& pos) {
    if (tok.kind != PPTokenKind::CharConst) return std::nullopt;

    std::string ch = tok.lexeme;
    if (ch.size() >= 3 && ch.front() == '\'' && ch.back() == '\'') {
        ch = ch.substr(1, ch.size() - 2);
        if (ch.size() == 1) {
            pos++;
            // Character constants have type int (signed).
            return Value{ static_cast<uint64_t>(static_cast<unsigned char>(ch[0])), false };
        }
    }
    diagnostics.push_back(Diagnostic{
        .message = std::string("invalid character constant in expression: ") + tok.lexeme,
        .severity = Diagnostic::Severity::Error,
        .span = tok.span
    });
    return std::nullopt;
}

std::optional<ConstExprParser::Value> ConstExprParser::tryParseParenExpr(const std::vector<PPToken>& tokens,
                                                          size_t end, size_t& pos) {
    if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "(") {
        return std::nullopt;
    }

    pos++;
    auto val = parseExpr(tokens, end, pos);
    if (!val) return std::nullopt;

    skipWhitespace(tokens, end, pos);
    if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != ")") {
        diagnostics.push_back(Diagnostic{
            .message = "expected ')' after expression",
            .severity = Diagnostic::Severity::Error,
            .span = (pos < end) ? std::optional<SourceSpan>(tokens[pos].span) : std::optional<SourceSpan>()
        });
        return std::nullopt;
    }
    pos++;
    return val;
}

bool ConstExprParser::parseDefinedMacroName(const std::vector<PPToken>& tokens, size_t end,
                                            size_t& pos, const SourceSpan& tokSpan,
                                            std::string& macroName) {
    if (pos >= end || tokens[pos].kind != PPTokenKind::Identifier) {
        diagnostics.push_back(Diagnostic{
            .message = "expected macro name in 'defined'",
            .severity = Diagnostic::Severity::Error,
            .span = (pos < end) ? tokens[pos].span : tokSpan
        });
        return false;
    }
    macroName = tokens[pos].lexeme;
    pos++;
    return true;
}

bool ConstExprParser::checkDefinedCloseParen(const std::vector<PPToken>& tokens,
                                              size_t end, size_t& pos) {
    skipWhitespace(tokens, end, pos);
    if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != ")") {
        diagnostics.push_back(Diagnostic{
            .message = "expected ')' after macro name in 'defined'",
            .severity = Diagnostic::Severity::Error,
            .span = (pos < end) ? std::optional<SourceSpan>(tokens[pos].span) : std::optional<SourceSpan>()
        });
        return false;
    }
    pos++;
    return true;
}

std::optional<ConstExprParser::Value> ConstExprParser::tryParseDefined(const std::vector<PPToken>& tokens,
                                                        size_t end, size_t& pos) {
    if (pos >= end || tokens[pos].kind != PPTokenKind::Identifier || tokens[pos].lexeme != "defined") {
        return std::nullopt;
    }

    const auto& tok = tokens[pos];
    pos++;
    skipWhitespace(tokens, end, pos);

    if (pos >= end) {
        diagnostics.push_back(Diagnostic{
            .message = "expected '(' or identifier after 'defined'",
            .severity = Diagnostic::Severity::Error,
            .span = tok.span
        });
        return std::nullopt;
    }

    bool hasParens = false;
    if (tokens[pos].kind == PPTokenKind::Punctuator && tokens[pos].lexeme == "(") {
        hasParens = true;
        pos++;
        skipWhitespace(tokens, end, pos);
    }

    std::string macroName;
    if (!parseDefinedMacroName(tokens, end, pos, tok.span, macroName)) {
        return std::nullopt;
    }

    if (hasParens && !checkDefinedCloseParen(tokens, end, pos)) {
        return std::nullopt;
    }

    return Value{ macroTable.getMacro(macroName) ? 1u : 0u, false };
}

std::optional<ConstExprParser::Value> ConstExprParser::parsePrimary(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    skipWhitespace(tokens, end, pos);
    if (pos >= end) {
        diagnostics.push_back(Diagnostic{
            .message = "unexpected end of constant expression",
            .severity = Diagnostic::Severity::Error,
            .span = std::nullopt
        });
        return std::nullopt;
    }

    const auto& tok = tokens[pos];

    if (tok.kind == PPTokenKind::PPNumber) return tryParsePPNumber(tok, pos);
    if (tok.kind == PPTokenKind::CharConst) return tryParseCharConst(tok, pos);
    if (auto val = tryParseParenExpr(tokens, end, pos)) return val;
    if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "defined") {
        return tryParseDefined(tokens, end, pos);
    }

    if (tok.kind == PPTokenKind::Identifier) {
        // 6.10.1p4: any remaining identifier (after macro expansion and the
        // `defined` operator) is replaced by 0.
        pos++;
        return Value{ 0u, false };
    }

    diagnostics.push_back(Diagnostic{
        .message = std::string("unexpected token in constant expression: ") + tok.lexeme,
        .severity = Diagnostic::Severity::Error,
        .span = tok.span
    });
    return std::nullopt;
}

} // namespace wvmcc
