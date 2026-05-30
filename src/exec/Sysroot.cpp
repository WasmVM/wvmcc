#include "Sysroot.hpp"

#include <climits>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace wvmcc {

namespace fs = std::filesystem;

std::optional<std::string> resolveSysroot(const SysrootEnv& env) {
    // Tier 1: --sysroot CLI flag.
    if (env.cliFlag && !env.cliFlag->empty()) {
        return *env.cliFlag;
    }

    // Tier 2: WVMCC_SYSROOT env var.
    if (env.envVar && env.envVar[0] != '\0') {
        return std::string(env.envVar);
    }

    // Tier 3: argv[0]-relative install layout.
    if (env.argv0 && env.argv0[0] != '\0') {
        std::error_code ec;
        fs::path exe(env.argv0);
        fs::path resolved = fs::weakly_canonical(exe, ec);
        if (ec) {
            // Best-effort: fall back to lexical dirname of argv[0].
            resolved = exe;
        }
        fs::path candidate = resolved.parent_path() / ".." / "share" / "wvmcc";
        candidate = candidate.lexically_normal();
        return candidate.string();
    }

    return std::nullopt;
}

} // namespace wvmcc
