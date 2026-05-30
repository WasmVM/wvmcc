#pragma once

#include "LinkContext.hpp"

namespace wvmcc::link::map {

// Write a human-readable linker map describing the final binary's
// contents to ctx.opts.map_path. No-op when map_path is empty.
//
// Format is plain text; deliberately not JSON or LLVM-style. Lists the
// inputs, function counts (broken down by import / defined), imports,
// exports, data segment ranges, and computed __heap_base.
void writeMap(LinkContext& ctx);

} // namespace wvmcc::link::map
