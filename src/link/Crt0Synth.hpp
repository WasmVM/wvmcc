#pragma once

#include "LinkContext.hpp"

namespace wvmcc::link::crt0 {

// Convert the merged linkable module into a self-contained one:
//
//   - Drop env.__linear_memory / env.__stack_memory / env.__stack_pointer /
//     env.__heap_base imports.
//   - Define them locally (mem[0] / mem[1] / globals[0] / globals[1]) so
//     the mem and global INDEX SPACES are unchanged — no instruction
//     rewriting needed for memory / global ops.
//   - Prepend the four sys_proc function imports (argc, argv_len, argv,
//     exit). Every existing function index reference shifts by +4 to
//     account for the new function imports.
//   - Append a start wrapper that calls `main` and forwards to
//     sys_proc.exit. Set module.start to it.
//
// Requires ctx.output to be a fully merged module (M2-L2) with an
// exported "main" function (wvmcc emits this hint in linkable mode).
// If "main" is absent, errors out unless opts.no_stdlib is set.
void synthesize(LinkContext& ctx);

} // namespace wvmcc::link::crt0
