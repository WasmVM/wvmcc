#include "Sysroot.hpp"

#include <climits>
#include <cstdlib>
#include <filesystem>
#include <string>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace wvmcc {

namespace fs = std::filesystem;

namespace {

// argv[0] with no directory component came from a PATH lookup — its lexical
// dirname is the CWD and says nothing about the install location (the packaged
// `wvmcc hello.c` case). Recover the real executable path from the OS, falling
// back to a PATH search; on a total miss return argv0 unchanged.
fs::path realExecutablePath(const fs::path& argv0) {
#if defined(__linux__)
    {
        std::error_code ec;
        fs::path self = fs::read_symlink("/proc/self/exe", ec);
        if (!ec && !self.empty()) {
            return self;
        }
    }
#elif defined(__APPLE__)
    {
        char buf[PATH_MAX];
        uint32_t size = sizeof(buf);
        if (_NSGetExecutablePath(buf, &size) == 0) {
            return fs::path(buf);
        }
    }
#endif
    if (const char* pathEnv = std::getenv("PATH")) {
        const std::string paths(pathEnv);
        std::size_t begin = 0;
        while (begin <= paths.size()) {
            const std::size_t end = paths.find(':', begin);
            const std::string dir = end == std::string::npos
                                        ? paths.substr(begin)
                                        : paths.substr(begin, end - begin);
            if (!dir.empty()) {
                std::error_code ec;
                fs::path candidate = fs::path(dir) / argv0;
                if (fs::exists(candidate, ec)) {
                    return candidate;
                }
            }
            if (end == std::string::npos) {
                break;
            }
            begin = end + 1;
        }
    }
    return argv0;
}

} // namespace

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
        fs::path exe(env.argv0);
        if (!exe.has_parent_path()) {
            exe = realExecutablePath(exe);
        }
        std::error_code ec;
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
