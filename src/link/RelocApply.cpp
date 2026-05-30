#include "RelocApply.hpp"

#include <algorithm>
#include <cstdint>
#include <variant>

namespace wvmcc::link::reloc {

namespace {

// Read the offset value out of a data segment's `mode.offset` ConstInstr.
// Returns 0 if the offset is missing or not a numeric const (which can
// happen for passive segments or imported-data forms we don't emit yet).
uint64_t readDataOffset(const WasmVM::WasmData& d) {
    if (!d.mode.offset.has_value()) return 0;
    uint64_t out = 0;
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::I64_const>) {
            out = (uint64_t)v.value;
        } else if constexpr (std::is_same_v<T, WasmVM::Instr::I32_const>) {
            out = (uint64_t)v.value;
        }
    }, *d.mode.offset);
    return out;
}

// Set the data segment's offset expression to a fresh i64 constant.
void setDataOffset(WasmVM::WasmData& d, uint64_t newOffset) {
    if (!d.mode.offset.has_value()) return; // passive: no offset to rewrite
    d.mode.offset = WasmVM::Instr::I64_const{(WasmVM::i64_t)newOffset};
}

// Round `v` up to a multiple of 8.
uint64_t alignUp8(uint64_t v) { return (v + 7u) & ~uint64_t{7}; }

} // namespace

void applyRelocations(LinkContext& ctx) {
    auto& datas = ctx.output.datas;
    if (datas.empty()) return;

    // Simple layout pass: assign each segment a non-overlapping range. We
    // sort by current offset to preserve intra-TU layout, then sweep
    // forward shifting any segment that would overlap the running top.
    //
    // For single-TU output this is a no-op (one segment, no shifts needed
    // because no other segment competes for the same address). For multi-
    // TU output it spreads the per-TU segments out.
    struct Idx {
        size_t i;
        uint64_t origOffset;
        uint64_t origEnd;
    };
    std::vector<Idx> sorted;
    sorted.reserve(datas.size());
    for (size_t i = 0; i < datas.size(); ++i) {
        uint64_t off = readDataOffset(datas[i]);
        sorted.push_back({i, off, off + datas[i].init.size()});
    }
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Idx& a, const Idx& b) {
                         return a.origOffset < b.origOffset;
                     });

    uint64_t topSoFar = 0;
    bool anyShifted = false;
    for (const auto& it : sorted) {
        uint64_t newOffset = std::max(it.origOffset, topSoFar);
        newOffset = alignUp8(newOffset);
        if (newOffset != it.origOffset) {
            setDataOffset(datas[it.i], newOffset);
            anyShifted = true;
        }
        topSoFar = newOffset + datas[it.i].init.size();
    }

    if (anyShifted) {
        ctx.note("  reloc-apply: shifted data segments to non-overlapping layout");
        ctx.note("  reloc-apply: TODO — rewrite i64.const data pointers in code "
                 "via per-TU reloc.CODE (needs codegen side-channel; harmless "
                 "single-TU is unaffected)");
    }
}

} // namespace wvmcc::link::reloc
