#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include <WasmVM.hpp>

static bool write_module_to_file(const WasmVM::WasmModule& module, const std::string& path) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        std::cerr << "error: cannot open output file: " << path << std::endl;
        return false;
    }
    WasmVM::module_encode(module, ofs);
    ofs.flush();
    std::cout << "wrote wasm to " << path << std::endl;
    return true;
}

int main(int argc, char** argv) {
    // Minimal CLI: support -o <file> to write encoded module
    std::string outPath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: wvmcc [options] [inputs]\n"
                         "\n"
                         "Options:\n"
                         "  -o <file>   Write output wasm to <file> (default: a.wasm)\n"
                         "  -h, --help  Show this help\n"
                      << std::endl;
            return 0;
        }
        if (arg == "-o" && i + 1 < argc) {
            outPath = argv[++i];
        }
        // Additional args (like input C source) will be wired later
    }

    WasmVM::WasmModule module;

    const std::string target = outPath.empty() ? std::string("a.wasm") : outPath;
    if (!write_module_to_file(module, target)) {
        return 1;
    }
    return 0;
}
