// M2-J unit test: 4-tier sysroot resolution.
//
//   1. --sysroot CLI flag wins outright
//   2. WVMCC_SYSROOT env var if no CLI flag
//   3. argv[0]-relative install layout (dirname/../share/wvmcc)
//   4. unset → std::nullopt
#include "../../../src/exec/Sysroot.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
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

int main() {
    using wvmcc::SysrootEnv;
    using wvmcc::resolveSysroot;

    // Tier 1: --sysroot wins over everything else.
    {
        SysrootEnv env;
        env.cliFlag = std::string("/cli/path");
        env.envVar = "/env/path";
        env.argv0 = "/usr/local/bin/wvmcc";
        auto got = resolveSysroot(env);
        EXPECT(got.has_value() && *got == "/cli/path",
               "tier 1: --sysroot must win");
    }

    // Tier 2: env var when no --sysroot.
    {
        SysrootEnv env;
        env.envVar = "/env/path";
        env.argv0 = "/usr/local/bin/wvmcc";
        auto got = resolveSysroot(env);
        EXPECT(got.has_value() && *got == "/env/path",
               "tier 2: WVMCC_SYSROOT must win when no --sysroot");
    }

    // Tier 2: empty env var must NOT count (treated as unset).
    {
        SysrootEnv env;
        env.envVar = "";
        env.argv0 = "/usr/local/bin/wvmcc";
        auto got = resolveSysroot(env);
        EXPECT(got.has_value(), "tier 3: argv[0]-relative kicks in for empty env var");
        EXPECT(got && got->find("share/wvmcc") != std::string::npos,
               "tier 3: argv[0]-relative path contains share/wvmcc");
    }

    // Tier 3: argv[0]-relative `dirname(argv0)/../share/wvmcc`.
    {
        SysrootEnv env;
        env.argv0 = "/opt/wvmcc/bin/wvmcc";
        auto got = resolveSysroot(env);
        EXPECT(got.has_value(), "tier 3: produces a path");
        // Expected: /opt/wvmcc/share/wvmcc (after lexical normalization)
        EXPECT(got && got->find("/opt/wvmcc/share/wvmcc") != std::string::npos,
               "tier 3: install-relative path resolves to share/wvmcc");
    }

    // Tier 3: a bare argv[0] (PATH-invoked) must NOT resolve against the CWD —
    // it is resolved to the real executable path (self-exe or PATH search), so
    // the result is an absolute install-relative path ending in share/wvmcc.
    {
        SysrootEnv env;
        env.argv0 = "wvmcc";
        auto got = resolveSysroot(env);
        EXPECT(got.has_value(), "tier 3 (bare argv0): produces a path");
        EXPECT(got && fs::path(*got).is_absolute(),
               "tier 3 (bare argv0): path is absolute, not CWD-relative");
        EXPECT(got && got->find("share/wvmcc") != std::string::npos,
               "tier 3 (bare argv0): path ends in share/wvmcc");
    }

    // Tier 4: no inputs → no sysroot.
    {
        SysrootEnv env;
        auto got = resolveSysroot(env);
        EXPECT(!got.has_value(), "tier 4: nothing set → std::nullopt");
    }

    // --sysroot with empty string falls through to next tier.
    {
        SysrootEnv env;
        env.cliFlag = std::string();
        env.envVar = "/env/path";
        auto got = resolveSysroot(env);
        EXPECT(got.has_value() && *got == "/env/path",
               "empty --sysroot falls through to env var");
    }

    if (failures == 0) {
        std::cout << "all sysroot-resolve tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
