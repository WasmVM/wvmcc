#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <deque>
#include <istream>
#include <unordered_set>
#include "../common.hpp"

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
    // Decoded metadata filled by the preprocessor Phase-5 normalization.
    // For character constants this holds the numeric value after decoding
    // escape sequences and UCNs. For string literals this holds the inner
    // decoded bytes (not including surrounding quotes or prefix).
    std::optional<uint32_t> decodedCharValue;
    std::optional<std::string> decodedString;
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
    
    // Helper methods for fill_buffer complexity reduction
    bool handlePendingSpace();
    bool handleCarriageReturn();
    bool processNormalState(char c);
    bool tryProcessComment(char c);
    bool tryProcessLineSplicing(char c);
    bool processStringState(char c);
    bool processCharState(char c);
    bool processBlockCommentState(char c);
    bool processLineCommentState(char c);
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
    
    // Helper methods for readNextToken - each handles one token type
    bool tryReadPPNumber(PPToken& out, char c, const SourcePos& begin);
    bool tryReadCharConstant(PPToken& out, const SourcePos& begin);
    bool tryReadStringLiteral(PPToken& out, const SourcePos& begin);
    bool tryReadNewline(PPToken& out, char c, const SourcePos& begin);
    bool tryReadWhitespace(PPToken& out, char c, const SourcePos& begin);
    bool tryReadIdentifier(PPToken& out, char c, const SourcePos& begin);
    bool tryReadPunctuator(PPToken& out, const SourcePos& begin);
    void readOtherToken(PPToken& out, const SourcePos& begin);
    
    // Helper methods for tryReadCharConstant - escape sequence processing
    bool processEscapeHex(std::string& lex, bool& escaped);
    bool processEscapeUnicode(std::string& lex, bool& escaped, int count);
    bool processEscapeOctal(std::string& lex, bool& escaped);
    bool processEscapedChar(char d, std::string& lex, bool& escaped);
    
    // Helper methods for tryReadPPNumber
    bool tryConsumeInitialDot(std::string& lex);
    bool tryConsumeInitialDigit(std::string& lex);
    bool processExponentNotation(char d, std::string& lex);
    void consumeDigitsAfterExponent(std::string& lex);
    
    // Helper methods for tryReadIdentifier
    bool canStartIdentifier(char c);
    bool tryConsumeIdentifierStart(char c, std::string& lex);
    bool consumeUCN(std::string& lex);
    void consumeIdentifierContinuation(std::string& lex);
    
    // Helper methods for tryReadStringLiteral
    void consumeStringPrefix(std::string& lex, size_t prefixLen);
    void consumeOpeningQuote(std::string& lex);
    void processStringContent(std::string& lex);
};

} // namespace wvmcc
