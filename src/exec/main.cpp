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
    // CLI: support -o <file>, -I<path> / -I <path>, and single input source path
    std::string outPath;
    std::optional<std::string> inputPath;
    std::vector<std::string> includePaths;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: wvmcc [options] <input>\n"
                         "\n"
                         "Options:\n"
                         "  -o <file>   Write output wasm to <file> (default: a.wasm)\n"
                         "  -I <path>   Add header search path (can repeat)\n"
                         "  -I<path>    Add header search path (attached form)\n"
                         "  -h, --help  Show this help\n"
                      << std::endl;
            return 0;
        }
        if (arg == "-o" && i + 1 < argc) {
            outPath = argv[++i];
            continue;
        }
        if (arg == "-I") {
            if (i + 1 < argc) {
                includePaths.push_back(argv[++i]);
                continue;
            } else {
                std::cerr << "error: -I requires a path\n";
                return 2;
            }
        }
        if (arg.rfind("-I", 0) == 0 && arg.size() > 2) {
            includePaths.push_back(arg.substr(2));
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
    for (const auto& p : includePaths) { pp.addIncludePath(p); }
    if (inputPath.has_value()) {
        auto res = pp.run(*inputPath);
        if (!res.success) {
            std::cerr << "preprocess error: " << res.errorMsg << std::endl;
            return 1;
        }
        // Print diagnostics collected during preprocessing
        for (const auto& d : pp.getDiagnostics()) {
            const char* sev = (d.severity == wvmcc::Diagnostic::Severity::Error) ? "error" :
                              (d.severity == wvmcc::Diagnostic::Severity::Warning) ? "warning" : "info";
            std::cerr << sev << ": " << d.message << "\n";
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
