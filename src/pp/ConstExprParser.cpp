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
    return result;
}

void ConstExprParser::skipWhitespace(const std::vector<PPToken>& tokens, size_t end, size_t& pos) const {
    while (pos < end && tokens[pos].kind == PPTokenKind::Whitespace) pos++;
}

std::optional<int64_t> ConstExprParser::parseExpr(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    return parseTernary(tokens, end, pos);
}

std::optional<int64_t> ConstExprParser::parseTernary(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
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
        return *val ? *trueVal : *falseVal;
    }
    return val;
}

std::optional<int64_t> ConstExprParser::parseLogicalOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseLogicalAnd(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "||") break;
        pos++;
        auto right = parseLogicalAnd(tokens, end, pos);
        if (!right) return std::nullopt;
        left = *left || *right;
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseLogicalAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseBitwiseOr(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "&&") break;
        pos++;
        auto right = parseBitwiseOr(tokens, end, pos);
        if (!right) return std::nullopt;
        left = *left && *right;
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseBitwiseOr(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseBitwiseXor(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "|" ||
            (pos + 1 < end && tokens[pos + 1].lexeme == "|")) break;
        pos++;
        auto right = parseBitwiseXor(tokens, end, pos);
        if (!right) return std::nullopt;
        left = *left | *right;
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseBitwiseXor(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseBitwiseAnd(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "^") break;
        pos++;
        auto right = parseBitwiseAnd(tokens, end, pos);
        if (!right) return std::nullopt;
        left = *left ^ *right;
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseBitwiseAnd(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    auto left = parseEquality(tokens, end, pos);
    if (!left) return std::nullopt;
    while (true) {
        skipWhitespace(tokens, end, pos);
        if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != "&" ||
            (pos + 1 < end && tokens[pos + 1].lexeme == "&")) break;
        pos++;
        auto right = parseEquality(tokens, end, pos);
        if (!right) return std::nullopt;
        left = *left & *right;
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseEquality(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
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
        left = (op == "==") ? (*left == *right) : (*left != *right);
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseRelational(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
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
        if (op == "<") left = *left < *right;
        else if (op == ">") left = *left > *right;
        else if (op == "<=") left = *left <= *right;
        else if (op == ">=") left = *left >= *right;
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseShift(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
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
        left = (op == "<<") ? (*left << *right) : (*left >> *right);
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseAdditive(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
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
        left = (op == "+") ? (*left + *right) : (*left - *right);
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseMultiplicative(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
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
        if (op == "*") left = *left * *right;
        else if (op == "/") {
            if (*right == 0) {
                diagnostics.push_back(Diagnostic{
                    .message = "division by zero in constant expression",
                    .severity = Diagnostic::Severity::Error,
                    .span = tokens[pos - 1].span
                });
                return std::nullopt;
            }
            left = *left / *right;
        } else if (op == "%") {
            if (*right == 0) {
                diagnostics.push_back(Diagnostic{
                    .message = "modulo by zero in constant expression",
                    .severity = Diagnostic::Severity::Error,
                    .span = tokens[pos - 1].span
                });
                return std::nullopt;
            }
            left = *left % *right;
        }
    }
    return left;
}

std::optional<int64_t> ConstExprParser::parseUnary(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
    skipWhitespace(tokens, end, pos);
    if (pos < end && tokens[pos].kind == PPTokenKind::Punctuator &&
        (tokens[pos].lexeme == "!" || tokens[pos].lexeme == "~" ||
         tokens[pos].lexeme == "+" || tokens[pos].lexeme == "-")) {
        std::string op = tokens[pos].lexeme;
        pos++;
        auto val = parseUnary(tokens, end, pos);
        if (!val) return std::nullopt;
        if (op == "!") return !*val;
        else if (op == "~") return ~*val;
        else if (op == "-") return -*val;
        else return *val;
    }
    return parsePrimary(tokens, end, pos);
}

std::optional<int64_t> ConstExprParser::parsePrimary(const std::vector<PPToken>& tokens, size_t end, size_t& pos) {
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

    if (tok.kind == PPTokenKind::PPNumber) {
        try {
            int64_t val = std::stoll(tok.lexeme, nullptr, 0);
            pos++;
            return val;
        } catch (...) {
            diagnostics.push_back(Diagnostic{
                .message = std::string("invalid integer constant: ") + tok.lexeme,
                .severity = Diagnostic::Severity::Error,
                .span = tok.span
            });
            return std::nullopt;
        }
    }

    if (tok.kind == PPTokenKind::CharConst) {
        std::string ch = tok.lexeme;
        if (ch.size() >= 3 && ch.front() == '\'' && ch.back() == '\'') {
            ch = ch.substr(1, ch.size() - 2);
            if (ch.size() == 1) {
                pos++;
                return static_cast<int64_t>(static_cast<unsigned char>(ch[0]));
            }
        }
        diagnostics.push_back(Diagnostic{
            .message = std::string("invalid character constant in expression: ") + tok.lexeme,
            .severity = Diagnostic::Severity::Error,
            .span = tok.span
        });
        return std::nullopt;
    }

    if (tok.kind == PPTokenKind::Punctuator && tok.lexeme == "(") {
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

    if (tok.kind == PPTokenKind::Identifier && tok.lexeme == "defined") {
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
        if (pos >= end || tokens[pos].kind != PPTokenKind::Identifier) {
            diagnostics.push_back(Diagnostic{
                .message = "expected macro name in 'defined'",
                .severity = Diagnostic::Severity::Error,
                .span = (pos < end) ? tokens[pos].span : tok.span
            });
            return std::nullopt;
        }
        std::string macroName = tokens[pos].lexeme;
        pos++;
        if (hasParens) {
            skipWhitespace(tokens, end, pos);
            if (pos >= end || tokens[pos].kind != PPTokenKind::Punctuator || tokens[pos].lexeme != ")") {
                diagnostics.push_back(Diagnostic{
                    .message = "expected ')' after macro name in 'defined'",
                    .severity = Diagnostic::Severity::Error,
                    .span = (pos < end) ? std::optional<SourceSpan>(tokens[pos].span) : std::optional<SourceSpan>()
                });
                return std::nullopt;
            }
            pos++;
        }
        return macroTable.getMacro(macroName) ? 1 : 0;
    }

    if (tok.kind == PPTokenKind::Identifier) {
        pos++;
        return 0;
    }

    diagnostics.push_back(Diagnostic{
        .message = std::string("unexpected token in constant expression: ") + tok.lexeme,
        .severity = Diagnostic::Severity::Error,
        .span = tok.span
    });
    return std::nullopt;
}

} // namespace wvmcc
