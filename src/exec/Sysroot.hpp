#pragma once

#include <optional>
#include <string>

namespace wvmcc {

struct SysrootEnv {
    // Value passed via `--sysroot=<path>` or `--sysroot <path>`.
    std::optional<std::string> cliFlag;
    // Value of the `WVMCC_SYSROOT` environment variable. May be nullptr.
    const char* envVar = nullptr;
    // `argv[0]` as provided to main. May be nullptr.
    const char* argv0 = nullptr;
};

// 4-tier resolution, first hit wins:
//   1. `--sysroot=<path>` CLI flag
//   2. `WVMCC_SYSROOT` environment variable
//   3. `dirname(realpath(argv[0]))/../share/wvmcc` (install-relative). A bare
//      argv[0] (no directory component — a PATH-invoked `wvmcc`) is first
//      resolved to the real executable path via the OS (/proc/self/exe or
//      _NSGetExecutablePath), falling back to a PATH search.
//   4. Unset
//
// Returns std::nullopt only on tier 4. Tiers 1–3 return the raw path even if
// the directory does not currently exist — the caller (preprocessor / linker)
// decides whether the missing path is fatal.
std::optional<std::string> resolveSysroot(const SysrootEnv& env);

} // namespace wvmcc
