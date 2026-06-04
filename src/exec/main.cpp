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
    std::optional<std::string> inputPath;
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

    if (!args.inputPath.has_value()) {
        std::cerr << "error: no input file specified\n";
        showHelp();
        return 2;
    }

    // Resolve sysroot (4-tier: --sysroot > WVMCC_SYSROOT > argv[0]-relative
    // > unset). The result feeds preprocessor include search (M2-I) and the
    // future link phase.
    wvmcc::SysrootEnv srEnv;
    srEnv.cliFlag = args.sysrootFlag;
    srEnv.envVar  = std::getenv("WVMCC_SYSROOT");
    srEnv.argv0   = argc > 0 ? argv[0] : nullptr;
    auto sysroot = wvmcc::resolveSysroot(srEnv);

    wvmcc::Preprocessor pp;
    for (const auto& p : args.includePaths) {
        pp.addIncludePath(p);
    }
    for (const auto& p : args.systemIncludePaths) {
        pp.addSystemIncludePath(p);
    }
    if (sysroot) {
        pp.setSysroot(*sysroot);
    }
    
    if (!pp.open(*args.inputPath)) {
        std::cerr << "preprocess error: failed to open input" << std::endl;
        return 1;
    }

    // Print preprocessor diagnostics collected while opening/processing includes
    printDiagnostics(pp.getDiagnostics());

    // If requested, run preprocessor-only mode: dump tokens' lexemes to stdout
    if (args.preprocessOnly) {
        while (auto tok = pp.next()) {
            std::cout << tok->lexeme;
        }
        return 0;
    }

    wvmcc::parser::Lexer lex(pp);
    using namespace wvmcc::parser;
    Parser parser(lex);
    TranslationUnitPtr main_translation_unit = parser.parseTranslationUnit();
    // print parser diagnostics if any
    printDiagnostics(parser.getDiagnostics());

    // If requested, write AST and exit immediately
    if (args.dumpAst) {
        std::string astPath = "ast.xml";
        const std::string &in = *args.inputPath;
        size_t slash = in.find_last_of("/\\");
        size_t dot = in.find_last_of('.');
        std::string base;
        if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) base = in.substr(0, dot);
        else base = in;
        astPath = base + "_ast.xml";

        std::ofstream ofs(astPath);
        if (!ofs) {
            std::cerr << "error: cannot open AST output file: " << astPath << std::endl;
            return 1;
        }
        wvmcc::parser::ASTPrinter printer(ofs);
        printer.print(main_translation_unit);
        ofs.flush();
        return 0;
    }

    // Not dumping AST: run semantic checks and continue compiler passes
    wvmcc::parser::Semantic sem(main_translation_unit, false);
    bool sem_ok = sem.run(parser.getDiagnosticsRef());
    if (!sem_ok) {
        printDiagnostics(parser.getDiagnostics());
        return 1;
    }

    wvmcc::codegen::ModuleCodegen codegen(sem);
    // M2-F: pass the explicit -ffreestanding flag down to codegen. Default
    // mode in M2-D is Linkable; -ffreestanding flips it back to M1's
    // self-contained layout.
    if (args.freestanding) {
        codegen.setCompileMode(wvmcc::codegen::CompileMode::Freestanding);
    }
    auto module = codegen.generate(main_translation_unit);

    // Surface codegen diagnostics first: a codegen error (unimplemented
    // construct, undeclared identifier, …) is the root cause, whereas the
    // subsequent module_validate failure is just its downstream symptom.
    const auto& codegenDiags = codegen.getDiagnostics();
    if (!codegenDiags.empty()) {
        printDiagnostics(codegenDiags);
        bool hasError = false;
        for (const auto& d : codegenDiags) {
            if (d.severity == wvmcc::Diagnostic::Severity::Error) { hasError = true; break; }
        }
        if (hasError) return 1;
    }

    if (auto err = WasmVM::module_validate(module)) {
        std::cerr << "error: module validation failed: " << err->what() << std::endl;
        return 1;
    }

    const std::string target = args.outPath.empty() ? std::string("a.wasm") : args.outPath;

    // Three output paths:
    //   -ffreestanding  → self-contained module straight to disk (M1 path)
    //   -c              → linkable object straight to disk (with reloc.CODE)
    //   neither         → run the integrated linker (M2-L1..L10)
    if (args.freestanding || args.compileOnly) {
        if (!write_module_to_file(module, target, &codegen)) return 1;
        return 0;
    }

    // Linker path.
    wvmcc::link::LinkOptions linkOpts;
    linkOpts.verbose = args.verbose;
    linkOpts.no_stdlib = args.noStdLib;
    linkOpts.map_path = args.mapPath;
    if (sysroot) linkOpts.sysroot = *sysroot;

    std::vector<wvmcc::link::LinkInput> linkInputs;
    {
        wvmcc::link::LinkInput::InMemoryModule mod{std::move(module), *args.inputPath, {}};
        // M2-L8: hand the linker this TU's data-pointer sites so it can shift
        // the i64.const constants if it rebases the TU's data.
        for (const auto& rel : codegen.getRelocations()) {
            mod.dataRelocs.push_back(
                {(uint32_t)rel.codeFuncIdx, (uint32_t)rel.instrIdx});
        }
        // #79: function-pointer sites — the linker rebases the embedded
        // funcref-table slot when merging per-TU tables.
        for (const auto& rel : codegen.getFuncPtrRelocs()) {
            mod.funcPtrRelocs.push_back(
                {(uint32_t)rel.codeFuncIdx, (uint32_t)rel.instrIdx});
        }
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
