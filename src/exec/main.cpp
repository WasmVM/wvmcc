#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <optional>

#include <WasmVM.hpp>
#include "../pp/Preprocessor.hpp"

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
    // Minimal CLI: support -o <file> and single input source path
    std::string outPath;
    std::optional<std::string> inputPath;
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
            continue;
        }
        // First non-flag arg treated as input C source path
        if (arg.size() > 0 && arg[0] != '-') {
            if (!inputPath.has_value()) {
                inputPath = arg;
            }
        }
    }

    // Preprocess input if provided (tokens for now)
    wvmcc::Preprocessor pp;
    if (inputPath.has_value()) {
        auto res = pp.run(*inputPath);
        if (!res.success) {
            std::cerr << "preprocess error: " << res.errorMsg << std::endl;
            return 1;
        }
        // Report token counts by kind, including punctuators
        size_t ws = 0, nl = 0, punct = 0, other = 0;
        for (const auto& t : res.tokens) {
            using K = wvmcc::PPTokenKind;
            switch (t.kind) {
                case K::Whitespace: ++ws; break;
                case K::Newline: ++nl; break;
                case K::Punctuator: ++punct; break;
                case K::Other: ++other; break;
                default: break;
            }
        }
        std::cout << "preprocess: tokens=" << res.tokens.size()
                  << " whitespace=" << ws << " newline=" << nl
                  << " punctuator=" << punct << " other=" << other << std::endl;
    }

    WasmVM::WasmModule module;

    const std::string target = outPath.empty() ? std::string("a.wasm") : outPath;
    if (!write_module_to_file(module, target)) {
        return 1;
    }
    return 0;
}
