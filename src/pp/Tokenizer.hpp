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
    static std::vector<PPToken> tokenize(const std::string& input);

private:
    static bool is_digit(char c);
    static bool is_nondigit(char c);
    static bool is_space(char c);
    static bool is_hex(char c);
    static bool is_oct(char c);
};

// Internal: SourceBuffer performs inline phase 1–3 preprocessing while feeding chars
class SourceBuffer {
public:
    explicit SourceBuffer(const std::string& input);
    bool next_char(char& outCh);
    void ensure_stream(std::string& stream, std::size_t upto);
    bool lastWhitespace() const { return lastEmittedWhitespace; }
    const SourcePos& position() const { return pos; }

private:
    enum class State { Normal, InString, InChar, InBlockComment, InLineComment };
    const std::string& inputRef;
    State st{State::Normal};
    bool esc{false};
    std::size_t rawIdx{0};
    bool lastEmittedWhitespace{false};
    bool inputEndsWithNewline{false};
    std::string charBuf;
    SourcePos pos{0,1,1,0};

    char trigraph_at(std::size_t idx) const;
    void fill_buffer();
};

} // namespace wvmcc
