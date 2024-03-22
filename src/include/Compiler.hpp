#ifndef WVMCC_Compiler_DEF
#define WVMCC_Compiler_DEF

#include <filesystem>
#include <vector>
#include <string>
#include <optional>

#include <WasmVM.hpp>

namespace WasmVM {

struct Compiler {

    Compiler(std::vector<std::filesystem::path> include_paths);
    WasmModule compile(std::filesystem::path source);

private:
    std::vector<std::filesystem::path> include_paths;
};

}

#endif