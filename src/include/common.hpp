#ifndef WVMCC_common_DEF
#define WVMCC_common_DEF

#include <filesystem>

namespace WasmVM {

struct SourcePos {
    std::filesystem::path file;
    size_t line;
    size_t col;
};

} // namespace WasmVM

#endif