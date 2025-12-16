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

struct CommandLineArgs {
    std::string outPath;
    std::optional<std::string> inputPath;
    std::vector<std::string> includePaths;
};

void showHelp() {
    std::cout << "Usage: wvmcc [options] <input>\n"
                 "\n"
                 "Options:\n"
                 "  -o <file>   Write output wasm to <file> (default: a.wasm)\n"
                 "  -I <path>   Add header search path (can repeat)\n"
                 "  -I<path>    Add header search path (attached form)\n"
                 "  -h, --help  Show this help\n"
              << std::endl;
}

bool parseIncludePath(int& i, int argc, char** argv, std::vector<std::string>& includePaths) {
    std::string arg = argv[i];
    if (arg == "-I") {
        if (i + 1 < argc) {
            includePaths.push_back(argv[++i]);
            return true;
        }
        std::cerr << "error: -I requires a path\n";
        return false;
    }
    if (arg.rfind("-I", 0) == 0 && arg.size() > 2) {
        includePaths.push_back(arg.substr(2));
        return true;
    }
    return false;
}

bool parseOutputPath(int& i, int argc, char** argv, std::string& outPath) {
    if (i + 1 < argc) {
        outPath = argv[++i];
        return true;
    }
    return false;
}

void parseInputPath(const std::string& arg, std::optional<std::string>& inputPath) {
    if (arg.size() > 0 && arg[0] != '-' && !inputPath.has_value()) {
        inputPath = arg;
    }
}

int parseCommandLine(int argc, char** argv, CommandLineArgs& args) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            showHelp();
            return 0;
        }
        if (arg == "-o") {
            if (!parseOutputPath(i, argc, argv, args.outPath)) {
                continue;
            }
            continue;
        }
        if (arg == "-I" || arg.rfind("-I", 0) == 0) {
            if (!parseIncludePath(i, argc, argv, args.includePaths)) {
                return 2;
            }
            continue;
        }
        parseInputPath(arg, args.inputPath);
    }
    return -1;
}

void printDiagnostics(const std::vector<wvmcc::Diagnostic>& diagnostics) {
    for (const auto& d : diagnostics) {
        const char* sev = (d.severity == wvmcc::Diagnostic::Severity::Error) ? "error" :
                          (d.severity == wvmcc::Diagnostic::Severity::Warning) ? "warning" : "info";
        std::cerr << sev << ": " << d.message << "\n";
    }
}

void printTokenStats(const std::vector<wvmcc::PPToken>& tokens) {
    size_t ws = 0, nl = 0, punct = 0, other = 0;
    for (const auto& t : tokens) {
        using K = wvmcc::PPTokenKind;
        switch (t.kind) {
            case K::Whitespace: ++ws; break;
            case K::Newline: ++nl; break;
            case K::Punctuator: ++punct; break;
            case K::Other: ++other; break;
            default: break;
        }
    }
    std::cout << "preprocess: tokens=" << tokens.size()
              << " whitespace=" << ws << " newline=" << nl
              << " punctuator=" << punct << " other=" << other << std::endl;
}

int main(int argc, char** argv) {
    CommandLineArgs args;
    int parseResult = parseCommandLine(argc, argv, args);
    if (parseResult >= 0) return parseResult;

    wvmcc::Preprocessor pp;
    for (const auto& p : args.includePaths) {
        pp.addIncludePath(p);
    }
    
    if (args.inputPath.has_value()) {
        auto res = pp.run(*args.inputPath);
        if (!res.success) {
            std::cerr << "preprocess error: " << res.errorMsg << std::endl;
            return 1;
        }
        printDiagnostics(pp.getDiagnostics());
        printTokenStats(res.tokens);
    }

    WasmVM::WasmModule module;
    const std::string target = args.outPath.empty() ? std::string("a.wasm") : args.outPath;
    if (!write_module_to_file(module, target)) {
        return 1;
    }
    return 0;
}
