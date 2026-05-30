#include "RelocApply.hpp"

#include <algorithm>
#include <cstdint>
#include <variant>

namespace wvmcc::link::reloc {

void applyRelocations(LinkContext& ctx) {
    // M2-L8 data rebasing now happens during merge (ModuleMerge::mergeOne):
    // each TU's data block is packed after the previously-merged TUs and its
    // data-pointer i64.const constants are shifted by the same delta via the
    // per-TU reloc records. That keeps reloc application co-located with the
    // index remap that needs the input-local reloc indices, so there is
    // nothing left to do here. Kept as a phase hook for future relocation
    // kinds.
    (void)ctx;
}

} // namespace wvmcc::link::reloc
