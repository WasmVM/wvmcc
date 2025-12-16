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
    // Tokenize Phase 1–3 processed input into minimal tokens: Whitespace, Newline, Other.
    static std::vector<PPToken> tokenize_minimal(const std::string& input);
};

} // namespace wvmcc
