// Lexer implementation moved out of header
#include "Lexer.hpp"
#include <unordered_set>
#include <limits>
#include <type_traits>

namespace wvmcc::parser {

static const std::unordered_set<std::string> keywords = {
    "auto","extern","short","while","break","float","signed",
    "_Alignas","case","for","sizeof","_Alignof","char","goto",
    "static","_Atomic","const","if","struct","_Bool","continue",
    "inline","switch","_Complex","default","int","typedef",
    "_Generic","do","long","union","_Imaginary","double","register",
    "unsigned","_Noreturn","else","restrict","void","_Static_assert",
    "enum","return","volatile","_Thread_local"
};

// Classification helper used internally by this translation unit.
static inline Token classify_local(const wvmcc::PPToken& pp) {
    using K = wvmcc::PPTokenKind;
    switch (pp.kind) {
        case K::Identifier:
            if (keywords.count(pp.lexeme)) return Token(KeywordToken{pp.lexeme}, pp.span);
            return Token(IdentifierToken{pp.lexeme}, pp.span);
        case K::PPNumber: {
            // Heuristic: if lexeme contains '.', 'e', 'E', 'p', or 'P' it's a floating constant
            const std::string& s = pp.lexeme;
            bool isFloat = false;
            for (char c : s) {
                if (c == '.' || c == 'e' || c == 'E' || c == 'p' || c == 'P') { isFloat = true; break; }
            }
            if (isFloat) {
                // parse optional floating suffix: f/F => float, l/L => long double, otherwise double
                FloatingToken ftok;
                ftok.lexeme = s;
                if (!s.empty()) {
                    char last = s.back();
                    if (last == 'f' || last == 'F') ftok.resolved = FloatingToken::ResolvedType::Float;
                    else if (last == 'l' || last == 'L') ftok.resolved = FloatingToken::ResolvedType::LongDouble;
                    else ftok.resolved = FloatingToken::ResolvedType::Double;
                }
                return Token(ftok, pp.span);
            }

            // Parse integer constant: split suffix (u/U, l/L, ll/LL) from digits
            size_t j = s.size();
            bool isUnsigned = false;
            int longCount = 0;
            // consume trailing u/U and l/L in any valid order
            while (j > 0) {
                char c = s[j-1];
                if (c == 'u' || c == 'U') { isUnsigned = true; --j; continue; }
                if (c == 'l' || c == 'L') {
                    // count up to two L's for long-long
                    int cnt = 0;
                    while (j > 0 && (s[j-1] == 'l' || s[j-1] == 'L')) {
                        ++cnt; --j; if (cnt == 2) break;
                    }
                    longCount = cnt;
                    continue;
                }
                break;
            }

            std::string digits = s.substr(0, j);
            IntegerInfo info;
            try {
                if (digits.size() >= 2 && (digits[0] == '0') && (digits[1] == 'x' || digits[1] == 'X')) {
                    info.base = IntegerInfo::Base::Hexadecimal;
                    std::string body = digits.substr(2);
                    info.value = std::stoull(body, nullptr, 16);
                } else if (digits.size() >= 1 && digits[0] == '0' && digits.size() > 1) {
                    info.base = IntegerInfo::Base::Octal;
                    std::string body = digits.substr(1);
                    if (body.empty()) info.value = 0; else info.value = std::stoull(body, nullptr, 8);
                } else {
                    info.base = IntegerInfo::Base::Decimal;
                    info.value = std::stoull(digits, nullptr, 10);
                }
            } catch (...) {
                // Fallback: zero value on parse error
                info.value = 0;
                if (!digits.empty() && digits[0] == '0') info.base = IntegerInfo::Base::Octal; else info.base = IntegerInfo::Base::Decimal;
            }
            info.flags = IntegerInfo::FLAG_NONE;
            if (isUnsigned) info.flags |= IntegerInfo::FLAG_UNSIGNED;
            if (longCount == 1) info.flags |= IntegerInfo::FLAG_LONG;
            if (longCount == 2) info.flags |= IntegerInfo::FLAG_LONG_LONG;
            info.lexeme = s;

            // Resolve semantic type according to C rules and host limits
            auto fitsSigned = [](std::uint64_t v, std::uint64_t max)->bool { return v <= max; };
            // helpers for max values (use 64-bit unsigned to compare against parsed uint64_t values)
            const std::uint64_t max_int = static_cast<std::uint64_t>(std::numeric_limits<int>::max());
            const std::uint64_t max_long = static_cast<std::uint64_t>(std::numeric_limits<long>::max());
            const std::uint64_t max_ll = static_cast<std::uint64_t>(std::numeric_limits<long long>::max());
            const std::uint64_t max_uint = static_cast<std::uint64_t>(std::numeric_limits<unsigned int>::max());
            const std::uint64_t max_ulong = static_cast<std::uint64_t>(std::numeric_limits<unsigned long>::max());
            const std::uint64_t max_ull = static_cast<std::uint64_t>(std::numeric_limits<unsigned long long>::max());

            std::uint64_t val = info.value;

            // Build candidate lists based on suffix flags and base per C table
            bool isU = (info.flags & IntegerInfo::FLAG_UNSIGNED) != 0;
            bool isL = (info.flags & IntegerInfo::FLAG_LONG) != 0;
            bool isLL = (info.flags & IntegerInfo::FLAG_LONG_LONG) != 0;

            // helper lambdas to set resolved type
            auto set = [&](IntegerInfo::ResolvedType r){ info.resolved = r; };

            if (!isU && !isL && !isLL) {
                // no suffix
                if (info.base == IntegerInfo::Base::Decimal) {
                    if (fitsSigned(val, max_int)) { set(IntegerInfo::ResolvedType::Int); }
                    else if (fitsSigned(val, max_long)) { set(IntegerInfo::ResolvedType::Long); }
                    else if (fitsSigned(val, max_ll)) { set(IntegerInfo::ResolvedType::LongLong); }
                    else {
                        // no extended integer support: mark as None
                        set(IntegerInfo::ResolvedType::None);
                    }
                } else {
                    // octal/hex: try signed int, unsigned int, long, unsigned long, long long, unsigned long long
                    if (fitsSigned(val, max_int)) { set(IntegerInfo::ResolvedType::Int); }
                    else if (val <= max_uint) { set(IntegerInfo::ResolvedType::UnsignedInt); }
                    else if (fitsSigned(val, max_long)) { set(IntegerInfo::ResolvedType::Long); }
                    else if (val <= max_ulong) { set(IntegerInfo::ResolvedType::UnsignedLong); }
                    else if (fitsSigned(val, max_ll)) { set(IntegerInfo::ResolvedType::LongLong); }
                    else if (val <= max_ull) { set(IntegerInfo::ResolvedType::UnsignedLongLong); }
                    else {
                        set(IntegerInfo::ResolvedType::None);
                    }
                }
            } else if (isU && !isL && !isLL) {
                // u suffix
                if (val <= max_uint) { set(IntegerInfo::ResolvedType::UnsignedInt); }
                else if (val <= max_ulong) { set(IntegerInfo::ResolvedType::UnsignedLong); }
                else if (val <= max_ull) { set(IntegerInfo::ResolvedType::UnsignedLongLong); }
                else { set(IntegerInfo::ResolvedType::None); }
            } else if (!isU && isL && !isLL) {
                // l suffix
                if (info.base == IntegerInfo::Base::Decimal) {
                    if (fitsSigned(val, max_long)) { set(IntegerInfo::ResolvedType::Long); }
                    else if (fitsSigned(val, max_ll)) { set(IntegerInfo::ResolvedType::LongLong); }
                    else { set(IntegerInfo::ResolvedType::None); }
                } else {
                    if (fitsSigned(val, max_long)) { set(IntegerInfo::ResolvedType::Long); }
                    else if (val <= max_ulong) { set(IntegerInfo::ResolvedType::UnsignedLong); }
                    else if (fitsSigned(val, max_ll)) { set(IntegerInfo::ResolvedType::LongLong); }
                    else if (val <= max_ull) { set(IntegerInfo::ResolvedType::UnsignedLongLong); }
                    else { set(IntegerInfo::ResolvedType::None); }
                }
            } else if (isU && isL && !isLL) {
                // u and l
                if (val <= max_ulong) { set(IntegerInfo::ResolvedType::UnsignedLong); }
                else if (val <= max_ull) { set(IntegerInfo::ResolvedType::UnsignedLongLong); }
                else { set(IntegerInfo::ResolvedType::None); }
            } else if (!isU && isLL) {
                // ll suffix
                if (fitsSigned(val, max_ll)) { set(IntegerInfo::ResolvedType::LongLong); }
                else if (val <= max_ull) { set(IntegerInfo::ResolvedType::UnsignedLongLong); }
                else { set(IntegerInfo::ResolvedType::None); }
            } else if (isU && isLL) {
                // u + ll
                if (val <= max_ull) { set(IntegerInfo::ResolvedType::UnsignedLongLong); }
                else { set(IntegerInfo::ResolvedType::None); }
            }

            return Token(IntegerToken{info}, pp.span);
        }
        case K::CharConst: {
            const std::string s = pp.lexeme; // includes optional prefix and quotes
            CharacterInfo info;
            info.lexeme = s;
            // determine prefix and resolved type
            size_t pos = 0;
            if (s.size() >= 2) {
                if (s[0] == 'L') { info.resolved = CharacterInfo::ResolvedType::WChar; pos = 1; }
                else if (s[0] == 'u') { info.resolved = CharacterInfo::ResolvedType::Char16; pos = 1; }
                else if (s[0] == 'U') { info.resolved = CharacterInfo::ResolvedType::Char32; pos = 1; }
            }

            // If the preprocessor provided a decoded numeric value, use it.
            if (pp.decodedCharValue.has_value()) {
                info.value = pp.decodedCharValue.value();
                return Token(CharacterToken{info}, pp.span);
            }

            // Fallback: try to derive from lexeme by extracting inner bytes (no escape decoding)
            if (pos >= s.size() || s[pos] != '\'') { /* malformed, leave defaults */ return Token(CharacterToken{info}, pp.span); }
            if (s.size() < pos + 3) { return Token(CharacterToken{info}, pp.span); }
            std::string inner = s.substr(pos + 1, s.size() - (pos + 2) - 0);
            unsigned int acc = 0;
            for (unsigned char uc : inner) acc = (acc << 8) | static_cast<unsigned int>(uc);
            info.value = acc;
            return Token(CharacterToken{info}, pp.span);
        }
        case K::StringLiteral:
            if (pp.decodedString.has_value()) return Token(StringLiteralToken{pp.decodedString.value()}, pp.span);
            return Token(StringLiteralToken{pp.lexeme}, pp.span);
        case K::Punctuator:
            return Token(PunctuatorToken{pp.lexeme}, pp.span);
        default:
            return Token(EndOfFileToken{}, pp.span);
    }
}

Lexer::Lexer(wvmcc::Preprocessor& ps) : pp(&ps) { refillLA(); }

std::optional<Token> Lexer::peek() {
    skipWhitespaceAndNewlines();
    if (!la) return std::nullopt;
    return classify_local(*la);
}

std::optional<Token> Lexer::next() {
    skipWhitespaceAndNewlines();
    if (!pp) return std::nullopt;
    auto ppTok = pp->next();
    la = std::nullopt;
    refillLA();
    if (!ppTok) return std::nullopt;
    auto tk = classify_local(*ppTok);
    if (tk.kind() == TokenKind::EndOfFile) return std::nullopt;
    return tk;
}

void Lexer::skipWhitespaceAndNewlines() {
    refillLA();
    while (la) {
        if (la->kind == wvmcc::PPTokenKind::Whitespace || la->kind == wvmcc::PPTokenKind::Newline) {
            if (pp) pp->next();
            la = std::nullopt;
            refillLA();
            continue;
        }
        break;
    }
}

bool Lexer::consumeIfPunctuator(const std::string& punct) {
    skipWhitespaceAndNewlines();
    refillLA();
    if (!la) return false;
    if (la->kind == wvmcc::PPTokenKind::Punctuator && la->lexeme == punct) {
        if (pp) pp->next();
        la = std::nullopt;
        refillLA();
        return true;
    }
    return false;
}

std::optional<std::pair<std::string, wvmcc::SourceSpan>> Lexer::consumeIfIdentifier() {
    skipWhitespaceAndNewlines();
    refillLA();
    if (!la) return std::nullopt;
    if (la->kind == wvmcc::PPTokenKind::Identifier) {
        auto lex = la->lexeme;
        auto sp = la->span;
        if (pp) pp->next();
        la = std::nullopt;
        refillLA();
        return std::make_pair(lex, sp);
    }
    return std::nullopt;
}

bool Lexer::expectIdentifier(std::string& out, wvmcc::SourceSpan* span) {
    skipWhitespaceAndNewlines();
    refillLA();
    if (!la) return false;
    if (la->kind == wvmcc::PPTokenKind::Identifier) {
        out = la->lexeme;
        if (span) *span = la->span;
        if (pp) pp->next();
        la = std::nullopt;
        refillLA();
        return true;
    }
    return false;
}

ExprPtr Lexer::consumeIdentifierAsExpr() {
    auto opt = consumeIfIdentifier();
    if (!opt) return nullptr;
    auto node = std::make_shared<IdentifierExpr>();
    node->name = opt->first;
    node->span = opt->second;
    return node;
}

void Lexer::refillLA() {
    if (!la) {
        if (pp) la = pp->peek();
    }
}

} // namespace wvmcc::parser
