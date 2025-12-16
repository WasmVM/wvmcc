#pragma once

#include <optional>
#include <string>
#include "Tokenizer.hpp"

namespace wvmcc {

struct Diagnostic {
    enum class Severity { Info, Warning, Error };
    std::string message;
    Severity severity{Severity::Error};
    std::optional<SourceSpan> span{};
};

} // namespace wvmcc
