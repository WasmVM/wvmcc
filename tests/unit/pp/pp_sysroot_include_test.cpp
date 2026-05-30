// M2-I unit test: verify the preprocessor resolves #include <...> against
// -isystem paths and <sysroot>/include/, with the documented priority.
#include "pp/Preprocessor.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static int failures = 0;

#define EXPECT(cond, msg)                                                     \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__,         \
                         __LINE__);                                           \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

static std::string drain(wvmcc::Preprocessor& pp) {
    std::string out;
    while (auto tok = pp.next()) {
        out += tok->lexeme;
    }
    return out;
}

static void write_file(const fs::path& p, const std::string& contents) {
    fs::create_directories(p.parent_path());
    std::ofstream ofs(p);
    ofs << contents;
}

int main() {
    fs::path tmpRoot = fs::temp_directory_path() / "wvmcc_pp_sysroot_test";
    fs::remove_all(tmpRoot);

    // Layout: each header drops a distinctive identifier into the parent
    // token stream (the preprocessor's macro propagation across includes is
    // a separate concern; using raw tokens makes this test about include
    // *resolution*).
    //
    //   userI/   contains "user.h"       → emits `from_userI`
    //   isystem/ contains "sysinc.h"     → emits `from_isystem`
    //   sysroot/include/ contains "sr.h" → emits `from_sysroot`
    fs::path userI    = tmpRoot / "userI";
    fs::path sysIncDir = tmpRoot / "isystem";
    fs::path sysroot   = tmpRoot / "sysroot";
    write_file(userI / "user.h",                    "int from_userI;\n");
    write_file(sysIncDir / "sysinc.h",              "int from_isystem;\n");
    write_file(sysroot / "include" / "sr.h",        "int from_sysroot;\n");

    fs::path mainC = tmpRoot / "main.c";
    write_file(mainC,
        "#include <user.h>\n"
        "#include <sysinc.h>\n"
        "#include <sr.h>\n");

    // Case 1: all three paths populated; each <...> resolves to its tier.
    {
        wvmcc::Preprocessor pp;
        pp.addIncludePath(userI.string());
        pp.addSystemIncludePath(sysIncDir.string());
        pp.setSysroot(sysroot.string());
        EXPECT(pp.open(mainC.string()), "open main.c");
        std::string out = drain(pp);
        EXPECT(out.find("from_userI") != std::string::npos,
               "<user.h> resolves via -I");
        EXPECT(out.find("from_isystem") != std::string::npos,
               "<sysinc.h> resolves via -isystem");
        EXPECT(out.find("from_sysroot") != std::string::npos,
               "<sr.h> resolves via sysroot");
    }

    // Case 2: -I takes precedence over -isystem.
    fs::path dupUserI = tmpRoot / "dup_userI";
    fs::path dupSysI  = tmpRoot / "dup_sysI";
    write_file(dupUserI / "dup.h", "int from_userI_dup;\n");
    write_file(dupSysI / "dup.h",  "int from_isystem_dup;\n");
    fs::path dupMain = tmpRoot / "dup.c";
    write_file(dupMain, "#include <dup.h>\n");
    {
        wvmcc::Preprocessor pp;
        pp.addIncludePath(dupUserI.string());
        pp.addSystemIncludePath(dupSysI.string());
        EXPECT(pp.open(dupMain.string()), "open dup.c");
        std::string out = drain(pp);
        EXPECT(out.find("from_userI_dup") != std::string::npos,
               "-I beats -isystem for the same header");
        EXPECT(out.find("from_isystem_dup") == std::string::npos,
               "-isystem version not chosen when -I has it");
    }

    // Case 3: unresolved <...> emits a diagnostic.
    {
        fs::path missingMain = tmpRoot / "missing.c";
        write_file(missingMain, "#include <does_not_exist.h>\n");
        wvmcc::Preprocessor pp;
        EXPECT(pp.open(missingMain.string()), "open missing.c");
        (void)drain(pp);
        const auto& diags = pp.getDiagnostics();
        bool hasError = false;
        for (const auto& d : diags) {
            if (d.severity == wvmcc::Diagnostic::Severity::Error) hasError = true;
        }
        EXPECT(hasError, "missing <...> produces an error diagnostic");
    }

    fs::remove_all(tmpRoot);
    if (failures == 0) {
        std::cout << "all pp-sysroot-include tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
