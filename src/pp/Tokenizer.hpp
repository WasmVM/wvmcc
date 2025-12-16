#pragma once

#include <string>
#include <vector>
#include <optional>

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

class SourceBuffer {
public:
    explicit SourceBuffer(const std::string& input);
    bool next_char(char& outCh);
    void ensure_stream(std::string& stream, std::size_t upto);
    bool lastWhitespace() const { return lastEmittedWhitespace; }
    const SourcePos& position() const { return pos; }
    void reset();

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

class Tokenizer {
public:
    explicit Tokenizer(const std::string& input);
    
    // Streaming API: read next token; returns std::nullopt at EOF
    std::optional<PPToken> next();
    // Lookahead: peek next token without consuming; std::nullopt at EOF
    std::optional<PPToken> peek();
    // Reset tokenizer state to the beginning of input
    void reset();
    
    // Optional convenience: check if we've reached end-of-input
    bool empty() {
        // Ensure at least one char is available if any remain
        feeder.ensure_stream(stream, streamPos);
        return streamPos >= stream.size();
    }

    // Range-style iteration support (consuming iterator)
    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = PPToken;
        using difference_type = std::ptrdiff_t;
        using pointer = const PPToken*;
        using reference = const PPToken&;

        Iterator() = default;
        explicit Iterator(Tokenizer* tz) : tz(tz) {
            advance();
        }

        reference operator*() const { return *current; }
        pointer operator->() const { return &(*current); }

        Iterator& operator++() { advance(); return *this; }
        Iterator operator++(int) { Iterator tmp = *this; advance(); return tmp; }

        bool operator==(const Iterator& other) const {
            if (eof && other.eof) return true;
            return tz == other.tz && eof == other.eof;
        }
        bool operator!=(const Iterator& other) const { return !(*this == other); }

    private:
        void advance();
        Tokenizer* tz{nullptr};
        bool eof{true};
        std::optional<PPToken> current;
    };

    Iterator begin();
    Iterator end();

private:
    static bool is_digit(char c);
    static bool is_nondigit(char c);
    static bool is_space(char c);
    static bool is_hex(char c);
    static bool is_oct(char c);

    // Member state for tokenization
    const std::string& input;
    SourceBuffer feeder;
    std::vector<PPToken> tokens;
    std::string stream;
    size_t streamPos;
    std::optional<PPToken> lookahead;

    void emit(PPTokenKind kind, const std::string& lexeme, SourcePos begin, SourcePos end);
    bool try_ucn(size_t idx, size_t& consumed);
    bool starts_char(size_t idx, size_t& prefixLen);
    bool starts_string(size_t idx, size_t& prefixLen);

    // Core worker to produce one token; returns false at EOF
    bool readNextToken(PPToken& out);
};

} // namespace wvmcc
