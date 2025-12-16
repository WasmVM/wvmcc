#pragma once

#include <string>
#include <vector>
#include <optional>
#include <deque>
#include "Tokenizer.hpp"
#include "MacroTable.hpp"

namespace wvmcc {

struct PreprocessResult {
    std::vector<PPToken> tokens;
    bool success{true};
    std::string errorMsg;
};

class Preprocessor {
public:
    PreprocessResult run(const std::string& inputPath);
    // Include search paths management
    void addIncludePath(const std::string& path) { includePaths.push_back(path); }
    void clearIncludePaths() { includePaths.clear(); }
    const std::vector<std::string>& getIncludePaths() const { return includePaths; }

    struct Diagnostic {
        enum class Severity { Info, Warning, Error };
        std::string message;
        Severity severity{Severity::Error};
        std::optional<SourceSpan> span{};
    };

    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

private:
    // Configurable -I paths (searched for quote includes after current file dir, and for angle includes)
    std::vector<std::string> includePaths{};
    // Queue of resolved include file paths to be executed in a later phase
    std::deque<std::string> includeQueue{};
    // Stack of files currently being processed (for cycle detection)
    std::vector<std::string> inclusionStack{};

    // Structured diagnostics collected during preprocessing
    std::vector<Diagnostic> diagnostics{};

    // Macro definitions
    MacroTable macroTable{};

    // Parse an #include directive payload starting right after the 'include' identifier.
    // Consumes whitespace and either <...> or "..." and emits a single HeaderName token into 'out'.
    // Execution/resolution is not performed here (parse only).
    // Returns true if include was executed (tokens from file emitted),
    // false if only a HeaderName token was emitted or nothing parsed.
    bool handleIncludeDirective(Tokenizer& tokenizer,
                                std::vector<PPToken>& out,
                                const std::string& currentDir);

    // Resolve include to a filesystem path according to search rules.
    // isAngle=true for <...> includes, false for "..." includes.
    // Returns a resolved absolute or canonical path if found, std::nullopt otherwise.
    std::optional<std::string> resolveInclude(const std::string& header,
                                              bool isAngle,
                                              const std::string& currentDir) const;

    // Check if a file is already in the inclusion stack (cycle detection).
    bool isInInclusionStack(const std::string& filePath) const;

    // Push file onto inclusion stack; returns false if already present (cycle detected).
    bool pushInclusion(const std::string& filePath);

    // Pop the current file from inclusion stack.
    void popInclusion();

    // Parse and execute a #define directive starting right after 'define' keyword.
    // Returns true if successful, false on error.
    bool handleDefineDirective(Tokenizer& tokenizer);

    // Parse and execute a #undef directive starting right after 'undef' keyword.
    // Returns true if successful, false on error.
    bool handleUndefDirective(Tokenizer& tokenizer);

    // Collect all tokens from current position until end of logical line (newline).
    // Does not consume the newline. Used by directive parsers.
    std::vector<PPToken> collectLineTokens(Tokenizer& tokenizer);

    // Expand object-like macros in a token stream.
    // Returns the expanded token list with macro substitutions applied.
    // Prevents infinite recursion by tracking expanded macro names.
    std::vector<PPToken> expandMacros(const std::vector<PPToken>& tokens);

    // Conditional compilation state tracking
    struct ConditionalFrame {
        bool seenTrueBranch{false};  // Has a taken #if/#elif been seen in this frame?
        bool currentlyActive{true};  // Should we emit tokens in this frame?
        bool inElse{false};          // Have we seen #else in this frame?
    };
    std::vector<ConditionalFrame> conditionalStack{};

    // Evaluate a C17 6.6 constant expression (integer-only, per preprocessor requirements).
    // Expands macros in tokens, then parses and evaluates as an integer expression.
    // Returns the evaluated result, or std::nullopt on parse/eval error (diagnostics emitted).
    std::optional<int64_t> evaluateConstantExpression(const std::vector<PPToken>& tokens);

    // Handle #if/#ifdef/#ifndef/#elif/#else/#endif directives.
    // Returns true if the directive was successfully parsed, false on error.
    bool handleIfDirective(Tokenizer& tokenizer, const std::vector<PPToken>& tokens);
    bool handleIfdefDirective(const std::string& macroName);
    bool handleIfndefDirective(const std::string& macroName);
    bool handleElifDirective(Tokenizer& tokenizer, const std::vector<PPToken>& tokens);
    bool handleElseDirective();
    bool handleEndifDirective();
};

} // namespace wvmcc
