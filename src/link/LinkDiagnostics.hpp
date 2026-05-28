#pragma once

#include "LinkContext.hpp"

namespace wvmcc::link::diag {

// Scan the post-resolve output for imports that are neither resolved nor
// in the expected host-runtime allow-list (sys_proc, sys_fs). Each is
// emitted as an error diagnostic with a "did you forget -l<libname>" hint.
//
// Collect-all-then-fail: every unresolved import gets its own error so
// users see them in one shot.
void emitUnresolvedDiagnostics(LinkContext& ctx);

} // namespace wvmcc::link::diag
