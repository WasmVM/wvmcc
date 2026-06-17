#pragma once

#include "LinkContext.hpp"
#include "Linker.hpp"
#include <WasmVM.hpp>

namespace wvmcc::link::merge {

// Per-input remap tables (input-index → output-index). Built as we merge
// each input module into ctx.output and applied to the input's instruction
// bodies as they're appended.
struct Remap {
    std::vector<WasmVM::index_t> type;
    std::vector<WasmVM::index_t> func;
    std::vector<WasmVM::index_t> global;
    std::vector<WasmVM::index_t> mem;
    std::vector<WasmVM::index_t> table;
};

// Append all of `in`'s contents to ctx.output, deduping imports / types and
// remapping every cross-reference. Records diagnostics on ctx on failure.
// `origin` is used in diagnostics. `dataRelocs` are the input's data-pointer
// sites (M2-L8): when `in`'s data segments are rebased to avoid colliding with
// previously-merged TUs, the `i64.const`s at these sites are shifted to match.
void mergeOne(LinkContext& ctx, const WasmVM::WasmModule& in,
              const std::string& origin,
              const std::vector<DataPtrSite>& dataRelocs = {},
              const std::vector<DataPtrSite>& funcPtrRelocs = {},
              const std::vector<DataSegPtrSite>& dataSegDataRelocs = {},
              const std::vector<DataSegPtrSite>& dataSegFuncPtrRelocs = {});

// Rewrite indices in a single instruction per `r`. Visible for unit tests.
void remapInstr(WasmVM::WasmInstr& instr, const Remap& r);

} // namespace wvmcc::link::merge
