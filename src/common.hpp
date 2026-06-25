// Common/shared types used across preprocessor and parser
#pragma once

#include <cstddef>
#include <string>
#include <optional>

namespace wvmcc {

struct SourcePos {
    int fileId = 0;
    int line = 0;
    int column = 0;
    std::size_t offset = 0;
};

struct SourceSpan {
    SourcePos begin;
    SourcePos end;
};

struct Diagnostic {
    enum class Severity { Info, Warning, Error };
    // Coarse phase classification (#28). Purely informational metadata; the
    // printed `error:`/`warning:` label is driven by `severity`, which also
    // controls the exit status. New fields are appended after `span` so the
    // existing designated-initializer call sites (`.message`, `.severity`,
    // `.span`) keep compiling in declaration order.
    enum class Category { Syntax, Semantic, Codegen, Warning };
    std::string message;
    Severity severity{Severity::Error};
    std::optional<SourceSpan> span{};
    // Optional actionable fix suggestion (#28); printed as a `note:` line.
    std::string hint{};
    Category category{Category::Syntax};
};

} // namespace wvmcc
