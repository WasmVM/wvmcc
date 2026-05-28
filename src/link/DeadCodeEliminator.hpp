#pragma once

#include "LinkContext.hpp"

namespace wvmcc::link::dce {

// Mark-and-sweep dead code elimination across the merged output.
//
// Marking:
//   * start function is reachable.
//   * every export (kind=func) is reachable.
//   * for each reachable function body, every Call / Return_call /
//     Ref_func target is reachable.
//   * Call_ref / Call_indirect: conservatively mark every function that
//     appears as a Ref_func anywhere (those are the funcs that can flow
//     through funcref values and therefore be the target of a call_ref).
//
// Sweep:
//   * remove unmarked defined functions.
//   * remap every funcidx in the module to the new (compact) layout.
//   * imports are never removed by DCE.
//
// For the default-mode single-TU compile everything is reachable, so the
// pass is a no-op. Becomes a real size reducer once libc TUs land.
void eliminate(LinkContext& ctx);

} // namespace wvmcc::link::dce
