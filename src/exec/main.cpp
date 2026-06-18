#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <optional>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

#include <WasmVM.hpp>
#include "../pp/Preprocessor.hpp"
#include "../parser/Lexer.hpp"
#include "../parser/ASTPrinter.hpp"
#include "../parser/AST.hpp"
#include "../parser/Parser.hpp"
#include "../parser/Semantic.hpp"
#include "../codegen/ModuleCodegen.hpp"
#include "../codegen/RelocSection.hpp"
#include "../link/Linker.hpp"
#include "../link/ArchiveReader.hpp"
#include "Sysroot.hpp"

static bool write_module_to_file(const WasmVM::WasmModule& module,
                                 const std::string& path,
                                 const wvmcc::codegen::ModuleCodegen* cg = nullptr) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        std::cerr << "error: cannot open output file: " << path << std::endl;
        return false;
    }
    WasmVM::module_encode(module, ofs);
    // M2-E: append linking / reloc.CODE custom sections in linkable mode.
    if (cg && cg->getCompileMode() == wvmcc::codegen::CompileMode::Linkable) {
        wvmcc::codegen::appendRelocSections(ofs, *cg);
    }
    ofs.flush();
    return true;
}

struct CommandLineArgs {
    std::string outPath;
    // All non-flag inputs in command-line order. May mix `.c` sources (compiled
    // to in-memory modules) and `.o`/`.wasm` objects (loaded as link inputs).
    std::vector<std::string> inputPaths;
    std::vector<std::string> includePaths;       // -I
    std::vector<std::string> systemIncludePaths; // -isystem
    std::vector<std::string> libraryPaths;       // -L
    std::vector<std::string> linkLibraries;      // -l<name>
    std::optional<std::string> sysrootFlag;      // --sysroot=<path>
    std::string mapPath;                         // --map=<path>
    bool dumpAst = false;
    bool preprocessOnly = false; // -E
    bool compileOnly = false;    // -c
    bool freestanding = false;   // -ffreestanding
    bool noStdLib = false;       // -nostdlib
    bool verbose = false;        // -v
};

void showHelp() {
    std::cout << "Usage: wvmcc [options] <input>\n"
                 "\n"
                 "Options:\n"
                 "  -o <file>       Write output wasm to <file> (default: a.wasm)\n"
                 "  -c              Compile only — emit a linkable .wasm object, no link\n"
                 "  -E              Run preprocessor only and write output to stdout\n"
                 "  -I <path>       Add header search path (can repeat; attached `-I<path>` ok)\n"
                 "  -isystem <dir>  Add system header search path (between -I and sysroot)\n"
                 "  -L <dir>        Add archive search path for -l\n"
                 "  -l<name>        Link `lib<name>.a` from -L or <sysroot>/lib (multiple ok)\n"
                 "  -nostdlib       Skip default libc linking and crt0 injection\n"
                 "  -ffreestanding  Emit a self-contained module (no linker step)\n"
                 "  -v              Verbose mode (link phase boundaries to stderr)\n"
                 "  --map=<path>    Write a linker map file describing the linked binary\n"
                 "  --sysroot=<dir> Override sysroot path (also: --sysroot <dir>)\n"
                 "                  Resolved in order: --sysroot > WVMCC_SYSROOT >\n"
                 "                  dirname(argv[0])/../share/wvmcc\n"
                 "  --ast           Dump a simple AST (XML) to stdout\n"
                 "  -h, --help      Show this help\n"
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

void parseInputPath(const std::string& arg, std::vector<std::string>& inputPaths) {
    if (arg.size() > 0 && arg[0] != '-') {
        inputPaths.push_back(arg);
    }
}

int parseCommandLine(int argc, char** argv, CommandLineArgs& args) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            showHelp();
            return 0;
        }
        if (arg == "--ast") {
            args.dumpAst = true;
            continue;
        }
        if (arg == "-E") {
            args.preprocessOnly = true;
            continue;
        }
        if (arg == "-c") {
            args.compileOnly = true;
            continue;
        }
        if (arg == "-ffreestanding") {
            args.freestanding = true;
            continue;
        }
        if (arg == "-nostdlib") {
            args.noStdLib = true;
            continue;
        }
        if (arg == "-v") {
            args.verbose = true;
            continue;
        }
        if (arg.rfind("--map=", 0) == 0) {
            args.mapPath = arg.substr(std::string("--map=").size());
            continue;
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
        // -isystem <dir>
        if (arg == "-isystem") {
            if (i + 1 < argc) {
                args.systemIncludePaths.push_back(argv[++i]);
            } else {
                std::cerr << "error: -isystem requires a path\n";
                return 2;
            }
            continue;
        }
        // -L <dir> or -L<dir>
        if (arg == "-L") {
            if (i + 1 < argc) {
                args.libraryPaths.push_back(argv[++i]);
            } else {
                std::cerr << "error: -L requires a path\n";
                return 2;
            }
            continue;
        }
        if (arg.rfind("-L", 0) == 0 && arg.size() > 2) {
            args.libraryPaths.push_back(arg.substr(2));
            continue;
        }
        // -l<name> (attached form only; -l with separate name is rare)
        if (arg.rfind("-l", 0) == 0 && arg.size() > 2) {
            args.linkLibraries.push_back(arg.substr(2));
            continue;
        }
        // --sysroot=<dir> or --sysroot <dir>
        if (arg == "--sysroot") {
            if (i + 1 < argc) {
                args.sysrootFlag = std::string(argv[++i]);
            } else {
                std::cerr << "error: --sysroot requires a path\n";
                return 2;
            }
            continue;
        }
        if (arg.rfind("--sysroot=", 0) == 0) {
            args.sysrootFlag = arg.substr(std::string("--sysroot=").size());
            continue;
        }
        parseInputPath(arg, args.inputPaths);
    }
    return -1;
}

// Classify a command-line input by extension. `.c` (and `.i`, a preprocessed
// source) is compiled; `.o`/`.wasm`/`.obj` is a linkable object loaded into the
// link directly. Unknown extensions default to "compile as C source", matching
// the historical single-input behaviour.
enum class InputKind { Source, Object };
InputKind classifyInput(const std::string& path) {
    auto dot = path.find_last_of('.');
    auto slash = path.find_last_of("/\\");
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return InputKind::Source; // no extension -> treat as source
    }
    std::string ext = path.substr(dot);
    if (ext == ".o" || ext == ".wasm" || ext == ".obj") return InputKind::Object;
    return InputKind::Source;
}

void printDiagnostics(const std::vector<wvmcc::Diagnostic>& diagnostics) {
    for (const auto& d : diagnostics) {
        const char* sev = (d.severity == wvmcc::Diagnostic::Severity::Error) ? "error" :
                          (d.severity == wvmcc::Diagnostic::Severity::Warning) ? "warning" : "info";
        std::cerr << sev << ": " << d.message << "\n";
    }
}

// Exit-status policy, shared across phases (#80): any Error-severity diagnostic
// forces a non-zero exit. Mirrors the end-of-run scans semantic/codegen/linker
// already do, so preprocessor and parser errors stop the compile too. Warnings
// and infos do not count.
bool hasError(const std::vector<wvmcc::Diagnostic>& diagnostics) {
    for (const auto& d : diagnostics) {
        if (d.severity == wvmcc::Diagnostic::Severity::Error) return true;
    }
    return false;
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

// Outcome of compiling one `.c` translation unit. `ok==false` means a
// diagnostic was already printed and the caller should exit non-zero. The
// reloc vectors are copied out of the (transient) ModuleCodegen so they can be
// handed to the linker after codegen is destroyed.
struct CompileResult {
    bool ok = false;
    WasmVM::WasmModule module;
    std::vector<wvmcc::link::DataPtrSite> dataRelocs;
    std::vector<wvmcc::link::DataPtrSite> funcPtrRelocs;
    // LANG-6.6-06: address-constant pointers baked into data segments.
    std::vector<wvmcc::link::DataSegPtrSite> dataSegDataRelocs;
    std::vector<wvmcc::link::DataSegPtrSite> dataSegFuncPtrRelocs;
};

// Compile a single source TU (preprocess → parse → semantic → codegen) into a
// WasmModule, in the given compile mode. Diagnostics are printed here; on the
// first error `ok` stays false. Each call gets a fresh Preprocessor so multiple
// TUs in one invocation do not share macro/include state.
static CompileResult compileSource(const std::string& path,
                                   const CommandLineArgs& args,
                                   const std::optional<std::string>& sysroot,
                                   bool freestanding) {
    CompileResult result;

    wvmcc::Preprocessor pp;
    for (const auto& p : args.includePaths) pp.addIncludePath(p);
    for (const auto& p : args.systemIncludePaths) pp.addSystemIncludePath(p);
    if (sysroot) pp.setSysroot(*sysroot);

    if (!pp.open(path)) {
        std::cerr << "preprocess error: failed to open input: " << path << std::endl;
        return result;
    }

    wvmcc::parser::Lexer lex(pp);
    wvmcc::parser::Parser parser(lex);
    wvmcc::parser::TranslationUnitPtr tu = parser.parseTranslationUnit();
    printDiagnostics(pp.getDiagnostics());
    printDiagnostics(parser.getDiagnostics());
    if (hasError(pp.getDiagnostics()) || hasError(parser.getDiagnostics())) {
        return result;
    }

    wvmcc::parser::Semantic sem(tu, false);
    if (!sem.run(parser.getDiagnosticsRef())) {
        printDiagnostics(parser.getDiagnostics());
        return result;
    }

    wvmcc::codegen::ModuleCodegen codegen(sem);
    codegen.setEnumConstants(parser.getEnumConstants());
    if (freestanding) {
        codegen.setCompileMode(wvmcc::codegen::CompileMode::Freestanding);
    }
    result.module = codegen.generate(tu);

    const auto& codegenDiags = codegen.getDiagnostics();
    if (!codegenDiags.empty()) {
        printDiagnostics(codegenDiags);
        for (const auto& d : codegenDiags) {
            if (d.severity == wvmcc::Diagnostic::Severity::Error) return result;
        }
    }

    if (auto err = WasmVM::module_validate(result.module)) {
        std::cerr << "error: module validation failed: " << err->what() << std::endl;
        return result;
    }

    // M2-L8 / #79: copy the data-pointer and function-pointer reloc sites out
    // of codegen so the linker can rebase them after this codegen is gone.
    for (const auto& rel : codegen.getRelocations()) {
        result.dataRelocs.push_back(
            {(uint32_t)rel.codeFuncIdx, (uint32_t)rel.instrIdx});
    }
    for (const auto& rel : codegen.getFuncPtrRelocs()) {
        result.funcPtrRelocs.push_back(
            {(uint32_t)rel.codeFuncIdx, (uint32_t)rel.instrIdx});
    }
    // LANG-6.6-06: address-constant pointers baked into data segments.
    for (const auto& rel : codegen.getDataSegDataRelocs()) {
        result.dataSegDataRelocs.push_back(
            {(uint32_t)rel.dataIndex, (uint32_t)rel.byteOffset});
    }
    for (const auto& rel : codegen.getDataSegFuncPtrRelocs()) {
        result.dataSegFuncPtrRelocs.push_back(
            {(uint32_t)rel.dataIndex, (uint32_t)rel.byteOffset});
    }
    result.ok = true;
    return result;
}

int main(int argc, char** argv) {
    CommandLineArgs args;
    int parseResult = parseCommandLine(argc, argv, args);
    if (parseResult >= 0) return parseResult;

    if (args.inputPaths.empty()) {
        std::cerr << "error: no input file specified\n";
        showHelp();
        return 2;
    }

    // Resolve sysroot (4-tier: --sysroot > WVMCC_SYSROOT > argv[0]-relative
    // > unset). The result feeds preprocessor include search (M2-I) and the
    // link phase.
    wvmcc::SysrootEnv srEnv;
    srEnv.cliFlag = args.sysrootFlag;
    srEnv.envVar  = std::getenv("WVMCC_SYSROOT");
    srEnv.argv0   = argc > 0 ? argv[0] : nullptr;
    auto sysroot = wvmcc::resolveSysroot(srEnv);

    // ---------------------------------------------------------------------
    // Single-source early-exit modes (-E, --ast). These act on exactly one
    // source TU and write to stdout / an AST file; they are not link inputs.
    // ---------------------------------------------------------------------
    if (args.preprocessOnly || args.dumpAst) {
        if (args.inputPaths.size() != 1) {
            std::cerr << "error: " << (args.preprocessOnly ? "-E" : "--ast")
                      << " accepts a single input file\n";
            return 2;
        }
        const std::string& input = args.inputPaths.front();

        wvmcc::Preprocessor pp;
        for (const auto& p : args.includePaths) pp.addIncludePath(p);
        for (const auto& p : args.systemIncludePaths) pp.addSystemIncludePath(p);
        if (sysroot) pp.setSysroot(*sysroot);
        if (!pp.open(input)) {
            std::cerr << "preprocess error: failed to open input" << std::endl;
            return 1;
        }

        if (args.preprocessOnly) {
            while (auto tok = pp.next()) std::cout << tok->lexeme;
            printDiagnostics(pp.getDiagnostics());
            return hasError(pp.getDiagnostics()) ? 1 : 0;
        }

        // --ast
        wvmcc::parser::Lexer lex(pp);
        wvmcc::parser::Parser parser(lex);
        wvmcc::parser::TranslationUnitPtr tu = parser.parseTranslationUnit();
        printDiagnostics(pp.getDiagnostics());
        printDiagnostics(parser.getDiagnostics());
        if (hasError(pp.getDiagnostics()) || hasError(parser.getDiagnostics())) {
            return 1;
        }
        size_t slash = input.find_last_of("/\\");
        size_t dot = input.find_last_of('.');
        std::string base =
            (dot != std::string::npos && (slash == std::string::npos || dot > slash))
                ? input.substr(0, dot) : input;
        std::string astPath = base + "_ast.xml";
        std::ofstream ofs(astPath);
        if (!ofs) {
            std::cerr << "error: cannot open AST output file: " << astPath << std::endl;
            return 1;
        }
        wvmcc::parser::ASTPrinter printer(ofs);
        printer.print(tu);
        ofs.flush();
        return 0;
    }

    // ---------------------------------------------------------------------
    // -ffreestanding / -c: emit one object per source straight to disk. These
    // are per-TU and never run the linker. With `-o`, only a single input is
    // allowed (the output name is unambiguous then). The codegen object is kept
    // alive here so write_module_to_file can append its reloc/linking sections.
    // ---------------------------------------------------------------------
    if (args.freestanding || args.compileOnly) {
        const char* mode = args.freestanding ? "-ffreestanding" : "-c";
        for (const auto& input : args.inputPaths) {
            if (classifyInput(input) == InputKind::Object) {
                std::cerr << "error: object input '" << input
                          << "' cannot be used with " << mode << "\n";
                return 2;
            }
        }
        if (args.inputPaths.size() != 1 && !args.outPath.empty()) {
            std::cerr << "error: cannot specify -o with multiple inputs in "
                      << mode << " mode\n";
            return 2;
        }
        for (const auto& input : args.inputPaths) {
            wvmcc::Preprocessor pp;
            for (const auto& p : args.includePaths) pp.addIncludePath(p);
            for (const auto& p : args.systemIncludePaths) pp.addSystemIncludePath(p);
            if (sysroot) pp.setSysroot(*sysroot);
            if (!pp.open(input)) {
                std::cerr << "preprocess error: failed to open input: " << input
                          << std::endl;
                return 1;
            }
            wvmcc::parser::Lexer lex(pp);
            wvmcc::parser::Parser parser(lex);
            wvmcc::parser::TranslationUnitPtr tu = parser.parseTranslationUnit();
            printDiagnostics(pp.getDiagnostics());
            printDiagnostics(parser.getDiagnostics());
            if (hasError(pp.getDiagnostics()) || hasError(parser.getDiagnostics())) {
                return 1;
            }
            wvmcc::parser::Semantic sem(tu, false);
            if (!sem.run(parser.getDiagnosticsRef())) {
                printDiagnostics(parser.getDiagnostics());
                return 1;
            }
            wvmcc::codegen::ModuleCodegen codegen(sem);
            codegen.setEnumConstants(parser.getEnumConstants());
            if (args.freestanding) {
                codegen.setCompileMode(wvmcc::codegen::CompileMode::Freestanding);
            }
            auto module = codegen.generate(tu);
            const auto& codegenDiags = codegen.getDiagnostics();
            if (!codegenDiags.empty()) {
                printDiagnostics(codegenDiags);
                for (const auto& d : codegenDiags) {
                    if (d.severity == wvmcc::Diagnostic::Severity::Error) return 1;
                }
            }
            if (auto err = WasmVM::module_validate(module)) {
                std::cerr << "error: module validation failed: " << err->what()
                          << std::endl;
                return 1;
            }
            std::string outFile = args.outPath;
            if (outFile.empty()) {
                size_t slash = input.find_last_of("/\\");
                size_t dot = input.find_last_of('.');
                std::string base =
                    (dot != std::string::npos &&
                     (slash == std::string::npos || dot > slash))
                        ? input.substr(0, dot) : input;
                outFile = base + (args.freestanding ? ".wasm" : ".o");
            }
            if (!write_module_to_file(module, outFile, &codegen)) return 1;
        }
        return 0;
    }

    // ---------------------------------------------------------------------
    // Link path: compile every `.c` source, load every `.o`/`.wasm` object,
    // then run the integrated linker over all of them plus the archives.
    // ---------------------------------------------------------------------
    const std::string target = args.outPath.empty() ? std::string("a.wasm") : args.outPath;

    wvmcc::link::LinkOptions linkOpts;
    linkOpts.verbose = args.verbose;
    linkOpts.no_stdlib = args.noStdLib;
    linkOpts.map_path = args.mapPath;
    if (sysroot) linkOpts.sysroot = *sysroot;

    std::vector<wvmcc::link::LinkInput> linkInputs;
    for (const auto& input : args.inputPaths) {
        if (classifyInput(input) == InputKind::Object) {
            // Object input: decode + parse its reloc sections, link directly.
            std::string err;
            auto obj = wvmcc::link::loadObjectFile(input, err);
            if (!obj) {
                std::cerr << "error: " << err << "\n";
                return 1;
            }
            wvmcc::link::LinkInput in;
            in.source = std::move(*obj);
            linkInputs.push_back(std::move(in));
            continue;
        }
        // Source input: compile to an in-memory linkable module.
        CompileResult cr = compileSource(input, args, sysroot, /*freestanding=*/false);
        if (!cr.ok) return 1;
        wvmcc::link::LinkInput::InMemoryModule mod{
            std::move(cr.module), input, std::move(cr.dataRelocs),
            std::move(cr.funcPtrRelocs), std::move(cr.dataSegDataRelocs),
            std::move(cr.dataSegFuncPtrRelocs)};
        wvmcc::link::LinkInput in;
        in.source = std::move(mod);
        linkInputs.push_back(std::move(in));
    }

    // Resolve a `-l<name>` to `lib<name>.a` under a `-L` dir or <sysroot>/lib.
    auto resolveLib = [&](const std::string& name) -> std::optional<std::string> {
        const std::string file = "lib" + name + ".a";
        for (const auto& dir : args.libraryPaths) {
            std::filesystem::path p = std::filesystem::path(dir) / file;
            if (std::filesystem::exists(p)) return p.string();
        }
        if (sysroot) {
            std::filesystem::path p =
                std::filesystem::path(*sysroot) / "lib" / file;
            if (std::filesystem::exists(p)) return p.string();
        }
        return std::nullopt;
    };

    // M2-L4: archive inputs. The linker pulls members lazily to satisfy
    // unresolved imports. Explicit `-l<name>` archives come first; libc is
    // appended last (default link) unless -nostdlib was given, mirroring cc.
    std::vector<std::string> libs = args.linkLibraries;
    if (!args.noStdLib) libs.push_back("c");
    for (const auto& lib : libs) {
        auto path = resolveLib(lib);
        if (!path) {
            // Missing libc with no sysroot is non-fatal (freestanding-style
            // builds link nothing); a missing explicit -l is an error.
            if (lib == "c" && !args.noStdLib &&
                std::find(args.linkLibraries.begin(), args.linkLibraries.end(),
                          "c") == args.linkLibraries.end()) {
                continue;
            }
            std::cerr << "error: cannot find -l" << lib
                      << " (lib" << lib << ".a) in -L paths or sysroot/lib\n";
            return 1;
        }
        wvmcc::link::LinkInput::ArchivePath ap{*path};
        wvmcc::link::LinkInput in;
        in.source = std::move(ap);
        linkInputs.push_back(std::move(in));
    }

    auto linkResult = wvmcc::link::link(std::move(linkInputs), linkOpts);

    if (args.verbose) {
        for (const auto& line : linkResult.log) {
            std::cerr << line << "\n";
        }
    }

    if (!linkResult.ok) {
        // Errors were appended to the log; surface them on stderr even
        // when not verbose.
        if (!args.verbose) {
            for (const auto& line : linkResult.log) {
                if (line.rfind("error:", 0) == 0) std::cerr << line << "\n";
            }
        }
        return 1;
    }

    if (auto err = WasmVM::module_validate(linkResult.module)) {
        std::cerr << "error: linked module validation failed: " << err->what() << std::endl;
        return 1;
    }

    // The linker output is the final binary — no reloc.CODE / linking
    // sections (M2-L8 strips them after applying), so pass nullptr for cg.
    if (!write_module_to_file(linkResult.module, target, nullptr)) return 1;
    return 0;
}
