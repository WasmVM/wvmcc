#pragma once

#include <string>
#include <vector>

namespace wvmcc {

enum class PPTokenKind {
    HeaderName,
    Identifier,
    PPNumber,
    CharConst,
    StringLiteral,
    Punctuator,
    Whitespace,
    Newline,
    Other
};

struct SourcePos { int fileId; int line; int column; std::size_t offset; };
struct SourceSpan { SourcePos begin; SourcePos end; };

struct PPToken {
    PPTokenKind kind;
    SourceSpan span;
    std::string lexeme;
};

class Tokenizer {
public:
    // Tokenize with punctuator recognition (greedy longest-match), plus Whitespace/Newline/Other.
    static std::vector<PPToken> tokenize_with_punctuators(const std::string& input);
    // Single-pass tokenizer: performs necessary phase-1–3 handling inline.
};

} // namespace wvmcc
