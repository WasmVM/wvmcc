#pragma once

#include <string>
#include <vector>
#include <optional>
#include <deque>
#include "Tokenizer.hpp"

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
};

} // namespace wvmcc
