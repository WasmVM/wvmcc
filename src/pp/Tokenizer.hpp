#pragma once

#include <string>
#include <optional>
#include <deque>
#include <istream>
#include <unordered_set>

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
    std::unordered_set<std::string> paintedMacros;  // Paint semantics: macros that have tried to expand this token
    
    // Check if this token is painted with a given macro name
    bool isPainted(const std::string& macroName) const {
        return paintedMacros.count(macroName) > 0;
    }
    
    // Mark this token as painted with a macro name
    void paint(const std::string& macroName) {
        paintedMacros.insert(macroName);
    }
    
    // Copy paint set from another token
    void copyPaint(const PPToken& other) {
        for (const auto& m : other.paintedMacros) {
            paintedMacros.insert(m);
        }
    }
};

class SourceBuffer {
public:
    explicit SourceBuffer(std::istream& in);
    bool next_char(char& outCh);
    const SourcePos& position() const { return pos; }
    void reset();
    
    // Ring-buffer based lookahead API
    // Ensure at least k characters available in lookahead buffer
    void ensure(std::size_t k);
    // Peek i-th lookahead character (0-based), std::nullopt if EOF before i
    std::optional<char> peek(std::size_t i);
    // Consume n characters from lookahead (returns true if fully consumed)
    bool consume(std::size_t n);
    // Get and consume one character from lookahead
    std::optional<char> get();

private:
    enum class State { Normal, InString, InChar, InBlockComment, InLineComment };
    std::istream& inStream;
    std::string inputAccum; // normalized input accumulated incrementally from inStream
    bool eof{false};
    State st{State::Normal};
    bool esc{false};
    std::size_t rawIdx{0};
    bool lastOutputWasWhitespace{false};
    std::string charBuf;
    std::deque<char> ring;
    bool pendingSpace{false};
    SourcePos pos{0,1,1,0};

    char trigraph_at(std::size_t idx) const;
    void fill_buffer();
    void ensure_input(std::size_t upto);
    void account_consumed(char ch);
};

class Tokenizer {
public:
    explicit Tokenizer(std::istream& in);
    
    // Streaming API: read next token; returns std::nullopt at EOF
    std::optional<PPToken> next();
    // Lookahead: peek next token without consuming; std::nullopt at EOF
    std::optional<PPToken> peek();
    // Reset tokenizer state to the beginning of input
    void reset();
    
    // Optional convenience: check if we've reached end-of-input
    bool empty() {
        return !feeder.peek(0).has_value();
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
    SourceBuffer feeder;
    std::optional<PPToken> lookahead;

    // no-op: emit helper removed in feeder-only design
    bool try_ucn(size_t idx, size_t& consumed);
    bool starts_char(size_t idx, size_t& prefixLen);
    bool starts_string(size_t idx, size_t& prefixLen);

    // Core worker to produce one token; returns false at EOF
    bool readNextToken(PPToken& out);
};

} // namespace wvmcc
