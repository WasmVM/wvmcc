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
    std::string message;
    Severity severity{Severity::Error};
    std::optional<SourceSpan> span{};
};

} // namespace wvmcc
