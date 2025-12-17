#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <optional>

#include <WasmVM.hpp>
#include "../pp/Preprocessor.hpp"
#include "../parser/Lexer.hpp"

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

void printTokenStats(const std::vector<wvmcc::parser::Token>& tokens) {
    size_t kw = 0, id = 0, intc = 0, floatc = 0, enumc = 0, chart = 0, str = 0, punct = 0;
    for (const auto& t : tokens) {
        std::visit([&](auto&& tok){
            using T = std::decay_t<decltype(tok)>;
            if constexpr (std::is_same_v<T, wvmcc::parser::KeywordToken>) ++kw;
            else if constexpr (std::is_same_v<T, wvmcc::parser::IdentifierToken>) ++id;
            else if constexpr (std::is_same_v<T, wvmcc::parser::IntegerToken>) ++intc;
            else if constexpr (std::is_same_v<T, wvmcc::parser::FloatingToken>) ++floatc;
            else if constexpr (std::is_same_v<T, wvmcc::parser::EnumerationToken>) ++enumc;
            else if constexpr (std::is_same_v<T, wvmcc::parser::CharacterToken>) ++chart;
            else if constexpr (std::is_same_v<T, wvmcc::parser::StringLiteralToken>) ++str;
            else if constexpr (std::is_same_v<T, wvmcc::parser::PunctuatorToken>) ++punct;
        }, t.v);
    }
    std::cout << "tokens=" << tokens.size()
              << " keywords=" << kw << " identifiers=" << id
              << " int_consts=" << intc << " float_consts=" << floatc << " enum_consts=" << enumc << " char_consts=" << chart
              << " strings=" << str << " punctuators=" << punct << std::endl;
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
        if (!pp.open(*args.inputPath)) {
            std::cerr << "preprocess error: failed to open input" << std::endl;
            return 1;
        }
        wvmcc::parser::Lexer lex(pp);
        std::vector<wvmcc::parser::Token> tokens;
        while (auto t = lex.next()) tokens.push_back(*t);
        printDiagnostics(pp.getDiagnostics());
        printTokenStats(tokens);
    }

    WasmVM::WasmModule module;
    const std::string target = args.outPath.empty() ? std::string("a.wasm") : args.outPath;
    if (!write_module_to_file(module, target)) {
        return 1;
    }
    return 0;
}
