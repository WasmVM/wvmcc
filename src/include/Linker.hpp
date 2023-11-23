#ifndef WVMCC_Linker_DEF
#define WVMCC_Linker_DEF

#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include <optional>

#include <WasmVM.hpp>

namespace WasmVM {

struct Linker {

    struct Config {
        std::vector<std::filesystem::path> library_paths;
        std::vector<std::string> libraries;
    };

    enum class LibraryType {
        Static, Shared
    };

    Linker(Config config);

private:
    WasmModule output;
    Config config;
    std::vector<std::pair<std::filesystem::path, LibraryType>> libraries;

    std::optional<LibraryType> check_library_type(std::filesystem::path path);
};

}

#endif