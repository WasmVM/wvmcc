#pragma once

#include <WasmVM.hpp>
#include <optional>
#include <ostream>
#include <string>
#include <vector>
#include <variant>

namespace wvmcc::link {

// One input to the linker. Either an in-memory module (a freshly compiled
// user TU) or an on-disk path to a wasmvm-ar archive ("libX.a"). Archive
// reading is lazy-loaded in M2-L4; for now archive inputs error out.
struct LinkInput {
    struct InMemoryModule {
        WasmVM::WasmModule module;
        std::string origin; // path or "<in-memory>" for diagnostics
    };
    struct ArchivePath {
        std::string path; // resolved filesystem path
    };
    std::variant<InMemoryModule, ArchivePath> source;
};

struct LinkOptions {
    bool verbose       = false;   // -v: log phase boundaries
    bool no_stdlib     = false;   // -nostdlib: skip default libc linking
    std::string map_path;         // M2-L10: --map=<path> to emit a map
    // Sysroot is resolved by the driver (M2-J) and passed in for archive
    // lookups. Unused in M2-L1 but stored for forward compatibility.
    std::string sysroot;
};

struct LinkResult {
    WasmVM::WasmModule module;
    bool ok = true;
    // Verbose log lines (always populated; printed by the driver when -v).
    std::vector<std::string> log;
};

// Run the integrated linker pipeline on `inputs` in command-line order.
// Returns the final WasmModule and a status. Diagnostics go to stderr via
// the driver (the linker collects them on LinkContext for the driver to
// flush — see LinkContext.hpp).
LinkResult link(std::vector<LinkInput> inputs, const LinkOptions& opts);

} // namespace wvmcc::link
