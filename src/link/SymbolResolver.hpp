#pragma once

#include "LinkContext.hpp"

namespace wvmcc::link::resolve {

// Walk the merged output's imports. For each function import:
//   * If its `module` is in the host-runtime allow-list (sys_proc, sys_fs):
//     leave it as a runtime-provided import.
//   * Otherwise, look up an exported function with the matching name; if
//     found, drop the import and rewrite every funcidx reference that
//     pointed at it to point at the resolved local index instead.
//
// Subsequent function indices (later imports + defined funcs) shift down
// by the number of resolved imports.
//
// Non-function imports are passed through unchanged in this issue;
// resolving cross-module memory / global / table imports lands when M2-L6
// crt0 needs to satisfy env.__* (which it currently does directly).
void resolveImports(LinkContext& ctx);

// Drop imports that nothing in the (post-DCE) module references and that are
// not runtime contracts (host modules / env runtime state). A declared-but-
// unused `extern` imposes no link requirement (C 6.9p5), so such an unresolved
// import must be removed rather than reported as undefined — leaving it would
// make the module fail to instantiate. Reindexes the function and global index
// spaces like resolveImports. Run after DCE (which may strip the last user of
// an import) and before diagnostics.
void pruneUnreferencedImports(LinkContext& ctx);

} // namespace wvmcc::link::resolve
