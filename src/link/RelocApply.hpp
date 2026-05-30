#pragma once

#include "LinkContext.hpp"

namespace wvmcc::link::reloc {

// Walk the merged output's data segments. If multiple data segments share
// an overlapping address range, shift each TU's segments to a unique
// non-overlapping range and update the offset expressions. For a single-
// TU output (the default-mode hello-world case), this is a no-op.
//
// After shifting, any `i64.const <addr>` in user code that originally
// referenced a shifted data symbol must also be rewritten — but doing so
// correctly requires the per-TU `reloc.CODE` records that wvmcc collects
// in `ModuleCodegen::getRelocations()`. Threading those through the link
// channel is non-trivial and only matters once we link real multi-TU
// programs (the libc TUs land in M2-1..M2-17). Until then, this stub
// only shifts data offsets; the i64.const-rewrite pass is a clearly-
// marked TODO.
void applyRelocations(LinkContext& ctx);

} // namespace wvmcc::link::reloc
