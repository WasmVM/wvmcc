// Common/shared types used across preprocessor and parser
#pragma once

#include <cstddef>

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

} // namespace wvmcc
