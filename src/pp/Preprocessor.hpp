#pragma once

#include <string>
#include <vector>
#include <optional>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include "Tokenizer.hpp"
#include "MacroTable.hpp"
#include "Diagnostics.hpp"

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

    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

private:
    // Configurable -I paths (searched for quote includes after current file dir, and for angle includes)
    std::vector<std::string> includePaths{};
    // Queue of resolved include file paths to be executed in a later phase
    std::deque<std::string> includeQueue{};
    // Stack of files currently being processed (for cycle detection)
    std::vector<std::string> inclusionStack{};
    
    // Include guard optimization: maps file path to its guard macro name
    // If a file has a detected guard and the macro is defined, we skip re-processing
    std::unordered_map<std::string, std::string> includeGuards{};
    
    // #pragma once optimization: set of files that have #pragma once
    // These files should only be processed once per translation unit
    std::unordered_set<std::string> pragmaOnceFiles{};

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
    bool parseMacroParameters(Tokenizer& tokenizer, std::vector<std::string>& params, bool& variadic);
    void trimReplacementWhitespace(std::vector<PPToken>& replacement);
    bool handleVariadicParameter(Tokenizer& tokenizer, bool& variadic);
    void skipToCloseParen(Tokenizer& tokenizer);

    // Parse and execute a #undef directive starting right after 'undef' keyword.
    // Returns true if successful, false on error.
    bool handleUndefDirective(Tokenizer& tokenizer);
    
    // Helper methods for handleIncludeDirective
    bool executeInclude(const std::string& header, bool isAngle,
                       std::optional<SourceSpan> span,
                       std::vector<PPToken>& out,
                       const std::string& currentDir);
    bool processAngleBracketInclude(Tokenizer& tokenizer,
                                    std::vector<PPToken>& out,
                                    const std::string& currentDir);
    bool processStringLiteralInclude(Tokenizer& tokenizer,
                                     std::vector<PPToken>& out,
                                     const std::string& currentDir);
    bool processMacroExpandedInclude(Tokenizer& tokenizer,
                                     std::vector<PPToken>& out,
                                     const std::string& currentDir);
    bool parseExpandedAngleBracket(const std::vector<PPToken>& expanded,
                                   size_t i,
                                   std::vector<PPToken>& out,
                                   const std::string& currentDir);
    bool parseExpandedStringLiteral(const PPToken& lit,
                                    std::vector<PPToken>& out,
                                    const std::string& currentDir);

    // Collect all tokens from current position until end of logical line (newline).
    // Does not consume the newline. Used by directive parsers.
    std::vector<PPToken> collectLineTokens(Tokenizer& tokenizer);

    // Expand object-like macros in a token stream.
    // Returns the expanded token list with macro substitutions applied.
    // Prevents infinite recursion by tracking expanded macro names.
    std::vector<PPToken> expandMacros(const std::vector<PPToken>& tokens);
    
    // Helper methods for expandMacros
    bool tryExpandFunctionLikeMacro(const std::vector<PPToken>& tokens, size_t& i, 
                                    const Macro* m, const PPToken& invocationToken,
                                    std::vector<PPToken>& result);
    std::vector<std::vector<PPToken>> collectMacroArguments(const std::vector<PPToken>& tokens,
                                                            size_t startIdx, size_t& endIdx,
                                                            const Macro* m);
    void handleOpenParen(std::vector<PPToken>& currentArg, const PPToken& tok, int& parenDepth);
    bool handleCloseParen(std::vector<PPToken>& currentArg, std::vector<std::vector<PPToken>>& args,
                         const PPToken& tok, int& parenDepth, size_t& endIdx, size_t k);
    void handleComma(std::vector<PPToken>& currentArg, std::vector<std::vector<PPToken>>& args);
    void collectVariadicArgs(std::vector<std::vector<PPToken>>& args, const Macro* m);
    std::vector<PPToken> substituteParameters(const Macro* m,
                                              const std::vector<std::vector<PPToken>>& args,
                                              const PPToken& invocationToken);
    bool tryProcessStringification(const Macro* m, size_t& rIdx,
                                   const std::vector<std::vector<PPToken>>& args,
                                   std::vector<PPToken>& substituted);
    int findParamIndex(const Macro* m, const PPToken& tok, bool& isVarargs);
    std::vector<PPToken> getArgumentToStringify(const Macro* m, int paramIdx, bool isVarargs,
                                               const std::vector<std::vector<PPToken>>& args);
    bool tryProcessVarArgs(const Macro* m, const PPToken& repl,
                          const std::vector<std::vector<PPToken>>& args,
                          std::vector<PPToken>& substituted);
    bool tryProcessRegularParam(const Macro* m, const PPToken& repl,
                               const std::vector<std::vector<PPToken>>& args,
                               std::vector<PPToken>& substituted);
    std::vector<PPToken> handleTokenPasting(const std::vector<PPToken>& tokens);
    std::string stringifyTokens(const std::vector<PPToken>& tokens);

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
    
    // Helper methods for evaluateConstantExpression
    size_t skipWhitespaceTokens(const std::vector<PPToken>& tokens, size_t start);
    bool tryParseDefinedOperator(const std::vector<PPToken>& tokens, size_t& i, std::vector<PPToken>& preprocessed);
    std::optional<std::string> parseDefinedMacroName(const std::vector<PPToken>& tokens, size_t& j, bool hasParens);
    bool validateDefinedCloseParen(const std::vector<PPToken>& tokens, size_t& j);

    // Handle #if/#ifdef/#ifndef/#elif/#else/#endif directives.
    // Returns true if the directive was successfully parsed, false on error.
    bool handleIfDirective(Tokenizer& tokenizer, const std::vector<PPToken>& tokens);
    bool handleIfdefDirective(const std::string& macroName);
    bool handleIfndefDirective(const std::string& macroName);
    bool handleElifDirective(Tokenizer& tokenizer, const std::vector<PPToken>& tokens);
    bool handleElseDirective();
    bool handleEndifDirective();
    
    // Handle utility directives
    // Returns true if the directive was successfully parsed, false on error.
    bool handleErrorDirective(const std::vector<PPToken>& tokens);
    bool handleWarningDirective(const std::vector<PPToken>& tokens);
    bool handleLineDirective(const std::vector<PPToken>& tokens);
    bool handlePragmaDirective(const std::vector<PPToken>& tokens);
    
    // Helper methods for handlePragmaDirective
    bool isPragmaOnceDirective(const std::vector<PPToken>& tokens);
    std::string buildPragmaContent(const std::vector<PPToken>& tokens);
    void trimTrailingWhitespace(std::string& content);

    // Refactoring helpers for run()
    bool validateLiteralToken(const PPToken& t, std::string& errMsg) const;
    bool processDirective(Tokenizer& tokenizer,
                          const PPToken& hashTok,
                          std::vector<PPToken>& out,
                          const std::string& currentDir,
                          bool& hasErrors);
    bool processLineStartToken(Tokenizer& tokenizer,
                               const PPToken& t,
                               std::vector<PPToken>& out,
                               const std::string& currentDir,
                               bool& hasErrors,
                               bool& atLineStart);
    void emitIfActive(const PPToken& t, std::vector<PPToken>& out) const;
    
    // Directive handlers
    bool handleDirectiveIfdef(Tokenizer& tokenizer, bool& hasErrors);
    bool handleDirectiveIfndef(Tokenizer& tokenizer, bool& hasErrors);
    bool handleDirectiveConditional(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors);
    bool handleDirectiveConditionalsGroup(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors);
    bool handleDirectiveMessage(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors);
    bool handleDirectiveLine(Tokenizer& tokenizer, bool& hasErrors);
    bool handleDirectivePragma(Tokenizer& tokenizer, bool& hasErrors);
    bool handleDirectiveDefineUndef(Tokenizer& tokenizer, const std::string& dirLexeme, bool& hasErrors);
    void consumeToEndOfLine(Tokenizer& tokenizer);
    
    // Helper methods for processDirective
    void skipDirectiveWhitespace(Tokenizer& tokenizer);
    bool checkActiveState();
    bool routeDirective(Tokenizer& tokenizer, const PPToken& hashTok, const PPToken& dir,
                       std::vector<PPToken>& out, const std::string& currentDir, bool& hasErrors);
    void consumeUnknownDirective(Tokenizer& tokenizer, std::vector<PPToken>& out);
    bool handleDirectiveInclude(Tokenizer& tokenizer, const PPToken& hashTok, 
                               const PPToken& dirToken, std::vector<PPToken>& out,
                               const std::string& currentDir, bool& hasErrors);
};

} // namespace wvmcc
