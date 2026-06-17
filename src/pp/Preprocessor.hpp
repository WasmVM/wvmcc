#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <sstream>
#include <filesystem>
#include "Tokenizer.hpp"
#include "MacroTable.hpp"
#include "../common.hpp"

namespace wvmcc {

class Preprocessor {
public:
    // --- Public streaming API -------------------------------------------------
    // Streaming interface only: treat Preprocessor as a live token stream.
    // The previous batch API (`run()`) has been removed.
    bool open(const std::string& inputPath);
    std::optional<PPToken> peek();
    std::optional<PPToken> next();
    void reset();
    bool empty();

    // Include path management
    void addIncludePath(const std::string& path) { includePaths.push_back(path); }
    void addSystemIncludePath(const std::string& path) { systemIncludePaths.push_back(path); }
    void setSysroot(const std::string& path) { sysroot = path; }
    void clearIncludePaths() { includePaths.clear(); }
    const std::vector<std::string>& getIncludePaths() const { return includePaths; }
    const std::vector<std::string>& getSystemIncludePaths() const { return systemIncludePaths; }
    const std::string& getSysroot() const { return sysroot; }

    // Diagnostics collected during preprocessing
    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

private:
    // --- Configuration & state -----------------------------------------------
    std::vector<std::string> includePaths{};           // -I search paths
    std::vector<std::string> systemIncludePaths{};     // -isystem search paths
    std::string sysroot{};                             // resolved sysroot (M2-J)
    std::vector<std::string> inclusionStack{};         // include cycle detection
    std::unordered_set<std::string> pragmaOnceFiles{}; // pragma once tracking
    std::vector<Diagnostic> diagnostics{};
    MacroTable macroTable{};
    // When pushing a *batch* of already-tokenized tokens back (e.g. via
    // handleInclude or macro expansion), pushTokenBackWithConcat must NOT
    // do tokenizer lookahead — the parent tokenizer points to source that
    // comes AFTER the batch in the output stream, so reading from it
    // splices unrelated tokens into the batch's string literals. Set this
    // flag while doing such a batch push.
    bool batchPushNoLookahead = false;

    // --- Include parsing & execution helpers --------------------------------
    // Parse-only: parse an #include payload (after the 'include' identifier).
    // Emits a HeaderName token into `out` when appropriate. Returns true if
    // the include was executed (file tokens emitted), false otherwise.
    bool handleIncludeDirective(Tokenizer& tokenizer,
                                std::vector<PPToken>& out,
                                const std::string& currentDir);

    // Resolve include names to filesystem paths. `isAngle=true` for <...>
    std::optional<std::string> resolveInclude(const std::string& header,
                                              bool isAngle,
                                              const std::string& currentDir) const;

    // Inclusion stack helpers
    bool isInInclusionStack(const std::string& filePath) const;
    bool pushInclusion(const std::string& filePath);
    void popInclusion();

    // Helpers used by include handling (angle vs string vs macro-expanded)
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

    // --- Directive parsing helpers (parse & execute specific directives) -----
    // Collect tokens until end of logical line (does not consume newline).
    std::vector<PPToken> collectLineTokens(Tokenizer& tokenizer);

    // #define / #undef parsing
    bool handleDefineDirective(Tokenizer& tokenizer);
    bool handleUndefDirective(Tokenizer& tokenizer);
    bool parseMacroParameters(Tokenizer& tokenizer, std::vector<std::string>& params, bool& variadic);
    void trimReplacementWhitespace(std::vector<PPToken>& replacement);
    bool handleVariadicParameter(Tokenizer& tokenizer, bool& variadic);
    void skipToCloseParen(Tokenizer& tokenizer);

    // #if / #ifdef / #ifndef / #elif / #else / #endif handling (parsers)
    bool handleIfDirective(Tokenizer& tokenizer, const std::vector<PPToken>& tokens);
    bool handleIfdefDirective(const std::string& macroName);
    bool handleIfndefDirective(const std::string& macroName);
    bool handleElifDirective(Tokenizer& tokenizer, const std::vector<PPToken>& tokens);
    bool handleElseDirective();
    bool handleEndifDirective();

    // Utility directives
    bool handleErrorDirective(const std::vector<PPToken>& tokens);
    bool handleWarningDirective(const std::vector<PPToken>& tokens);
    bool handleLineDirective(const std::vector<PPToken>& tokens);
    bool handlePragmaDirective(const std::vector<PPToken>& tokens);

    // Helpers for pragma parsing
    bool isPragmaOnceDirective(const std::vector<PPToken>& tokens);
    std::string buildPragmaContent(const std::vector<PPToken>& tokens);
    void trimTrailingWhitespace(std::string& content);

    // --- Macro expansion helpers ---------------------------------------------
    std::vector<PPToken> expandMacros(const std::vector<PPToken>& tokens);
    bool tryExpandFunctionLikeMacro(const std::vector<PPToken>& tokens, size_t& i,
                                    const Macro* m, const PPToken& invocationToken,
                                    std::vector<PPToken>& result);
    std::vector<std::vector<PPToken>> collectMacroArguments(const std::vector<PPToken>& tokens,
                                                            size_t startIdx, size_t& endIdx,
                                                            const Macro* m);
    void handleOpenParen(std::vector<PPToken>& currentArg, const PPToken& tok, int& parenDepth);
    struct CloseParenContext {
        int& parenDepth;
        size_t& endIdx;
        size_t k;
    };
    bool handleCloseParen(std::vector<PPToken>& currentArg, std::vector<std::vector<PPToken>>& args,
                                    const PPToken& tok, CloseParenContext& ctx);
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

    // --- Conditional evaluation helpers -------------------------------------
    struct ConditionalFrame {
        bool seenTrueBranch{false};
        bool currentlyActive{true};
        bool inElse{false};
    };
    std::vector<ConditionalFrame> conditionalStack{};

    std::optional<int64_t> evaluateConstantExpression(const std::vector<PPToken>& tokens);
    size_t skipWhitespaceTokens(const std::vector<PPToken>& tokens, size_t start);
    bool tryParseDefinedOperator(const std::vector<PPToken>& tokens, size_t& i, std::vector<PPToken>& preprocessed);
    std::optional<std::string> parseDefinedMacroName(const std::vector<PPToken>& tokens, size_t& j, bool hasParens);
    bool validateDefinedCloseParen(const std::vector<PPToken>& tokens, size_t& j);

    // --- Small utility helpers used by streaming implementation --------------
    bool checkActiveState();
    // Active state of all frames EXCEPT the current top frame — used by
    // #elif/#else when re-activating a branch, so the re-activated branch
    // still respects an inactive parent block.
    bool checkParentActiveState();
    void consumeToEndOfLine(Tokenizer& tokenizer);

    // --- Streaming implementation details -----------------------------------
    struct FileCtx {
        std::string path;
        std::unique_ptr<std::istringstream> stream;
        std::unique_ptr<Tokenizer> tokenizer;
        std::string dir;
    };

    std::vector<FileCtx> fileStack;
    std::deque<PPToken> outBuffer;
    bool atLineStart{true};

    bool pushFile(const std::string& path);
    std::optional<PPToken> readRawToken();
    void ensureBuffer();

    // Helpers extracted from ensureBuffer to reduce complexity of the hot path
    bool handleNewlineToken(const PPToken& tok);
    bool handleInactiveConditionalToken(const PPToken& tok);
    bool handleIdentifierExpansionToken(const PPToken& tok);
    bool tryHandleFunctionLikeMacroInvocation(const Macro* m, const PPToken& tok);
    void emitTokenOrLineExpansion(const PPToken& tok);
    // Normalize a literal token: decode escapes and UCNs in character and string literals
    void normalizeLiteralToken(PPToken& tok);
    void skipToEndOfLineTokensFromTokenizer(Tokenizer& tz);
    // Helpers to push tokens into the output buffer while performing Phase 6
    // adjacent string literal concatenation when applicable.
    void pushTokenBackWithConcat(const PPToken& tok);
    void pushTokenFrontWithConcat(const PPToken& tok);

    // Runtime-facing directive handlers used by the streaming `ensureBuffer`
    // Dispatch a directive. When `inactiveConditional` is true the line lies in a
    // skipped conditional branch, where only conditional-control directives
    // (#if/#ifdef/#ifndef/#elif/#else/#endif) run — every other directive
    // (#define, #undef, #include, #error, …) must be ignored (C 6.10p6).
    void handleDirective(bool inactiveConditional = false);
    void skipLineFromToken(PPToken t);
    void handleDefine();               // wrapper used by streaming layer
    void handleUndef();                // wrapper used by streaming layer
    void handleInclude();              // wrapper used by streaming layer
    void handleIfdef(bool wantDefined); // wrapper used by streaming layer
    // Helpers to break down large directive parsing logic
    bool handleIfOrElifDirective(const std::string& dir);
    bool handleUtilityDirective(const std::string& dir);
    void skipRestOfDirectiveTokens();
    bool handleSimpleDirective(const std::string& dir);
    std::vector<PPToken> collectInvocationTokens();
    bool consumeOptionalWhitespaceBeforeOpenParen();
    std::optional<std::string> readDirectiveName();
};

} // namespace wvmcc
