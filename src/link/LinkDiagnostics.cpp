#include "LinkDiagnostics.hpp"

#include <unordered_set>
#include <variant>

namespace wvmcc::link::diag {

namespace {

// Modules whose imports we always accept as unresolved — they're
// satisfied at instantiation time by the wasmvm host runtime.
const std::unordered_set<std::string> kHostModules = {"sys_proc", "sys_fs"};

// Names under `env` that are part of the crt0 contract. In normal link
// mode crt0 has already replaced these with local definitions; they only
// reach the diagnostics phase when the user passed -nostdlib, in which
// case it's explicit opt-out and not an error.
const std::unordered_set<std::string> kEnvRuntimeState = {
    "__linear_memory", "__stack_memory", "__stack_pointer",
    "__heap_base", "__indirect_function_table",
};

} // namespace

void emitUnresolvedDiagnostics(LinkContext& ctx) {
    const auto& imports = ctx.output.imports;
    for (const auto& imp : imports) {
        if (kHostModules.count(imp.module) > 0) continue;
        if (imp.module == "env" && kEnvRuntimeState.count(imp.name) > 0) continue;

        // Reach here only for imports M2-L3 didn't resolve — they really are
        // undefined references at this point.
        std::string sym = imp.module + "." + imp.name;
        std::string msg = "undefined reference to '" + sym + "'";

        // Add a helpful hint when the module name looks like an archive
        // namespace (anything other than "env" or a host module). For
        // env.<name>, the user probably forgot to declare or define the
        // function; for libc-like modules, suggest the archive.
        if (imp.module != "env") {
            msg += "\n  note: did you forget to link an archive? "
                   "Try -l" + imp.module;
        } else {
            msg += "\n  note: declare the function and provide its definition, "
                   "or link an archive that exports it";
        }

        ctx.error(std::move(msg));
    }
}

} // namespace wvmcc::link::diag
