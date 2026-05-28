#include "Linker.hpp"
#include "LinkContext.hpp"

#include <sstream>
#include <utility>
#include <variant>

namespace wvmcc::link {

// ---------------------------------------------------------------------------
// Phase stubs. Each lands in its own M2-L* issue; M2-L1 just wires the
// orchestration so the driver has a runnable plumbing skeleton.
// ---------------------------------------------------------------------------

namespace {

// M2-L2: merge each input WasmModule into ctx.output (type dedup + index
// remap + import/export concatenation). For L1 we implement a degenerate
// "first module wins" merge so single-input default-mode compiles still
// produce a valid wasm file. Multi-input merges are deferred.
void phaseMerge(LinkContext& ctx) {
    ctx.note("phase: merge");
    if (ctx.inputs.empty()) {
        ctx.error("link: no inputs");
        return;
    }
    int inMemoryCount = 0;
    for (const auto& in : ctx.inputs) {
        if (std::holds_alternative<LinkInput::InMemoryModule>(in.source)) {
            ++inMemoryCount;
        } else {
            ctx.error("link: archive inputs not yet supported (M2-L4 deferred until libc.a exists)");
        }
    }
    if (ctx.hasErrors()) return;

    if (inMemoryCount > 1) {
        // Full multi-input merge lands in M2-L2. For now, refuse so callers
        // get a clear message instead of silently picking the first module.
        ctx.error("link: multi-module merge not yet implemented (M2-L2)");
        return;
    }

    // Degenerate pass-through for the single-input case.
    const auto& first = std::get<LinkInput::InMemoryModule>(ctx.inputs[0].source);
    ctx.output = first.module;
    std::ostringstream ss;
    ss << "  merged 1 module (" << first.origin << ")";
    ctx.note(ss.str());
}

// M2-L7: rewrite per-TU funcref element segments into the merged shared
// __indirect_function_table layout. Stub for L1.
void phaseIndirectTableMerge(LinkContext& ctx) {
    ctx.note("phase: indirect-table-merge (stub)");
}

// M2-L8: rebase data segments + apply each TU's reloc.CODE entries. Stub
// for L1.
void phaseRelocApply(LinkContext& ctx) {
    ctx.note("phase: reloc-apply (stub)");
}

// M2-L6: synthesize crt0 (env.__* exports + start wrapper). Stub for L1 —
// the output therefore still has unresolved env.__* imports until L6.
void phaseCrt0(LinkContext& ctx) {
    ctx.note("phase: crt0-synth (stub)");
}

// M2-L3: resolve imports against the merged + crt0 exports. Stub for L1.
void phaseResolveImports(LinkContext& ctx) {
    ctx.note("phase: import-resolution (stub)");
}

// M2-L5: mark-and-sweep DCE across the merged output. Stub for L1.
void phaseDeadCodeElim(LinkContext& ctx) {
    ctx.note("phase: dce (stub)");
}

// M2-L9: report unresolved imports outside the allowed host-import list.
// Stub for L1.
void phaseDiagnostics(LinkContext& ctx) {
    ctx.note("phase: diagnostics (stub)");
}

// M2-L10: write --map=<path> output. Stub for L1.
void phaseMapOutput(LinkContext& ctx) {
    if (ctx.opts.map_path.empty()) return;
    ctx.note("phase: map-output (stub)");
}

} // namespace

LinkResult link(std::vector<LinkInput> inputs, const LinkOptions& opts) {
    LinkContext ctx;
    ctx.opts = opts;
    ctx.inputs = std::move(inputs);

    ctx.note("wvmcc link: starting (" + std::to_string(ctx.inputs.size()) + " input(s))");

    phaseMerge(ctx);
    if (!ctx.hasErrors()) {
        phaseIndirectTableMerge(ctx);
        phaseRelocApply(ctx);
        phaseCrt0(ctx);
        phaseResolveImports(ctx);
        phaseDeadCodeElim(ctx);
        phaseDiagnostics(ctx);
        phaseMapOutput(ctx);
    }

    LinkResult r;
    r.module = std::move(ctx.output);
    r.ok = !ctx.hasErrors();
    r.log = std::move(ctx.log);
    // Diagnostics live on ctx — propagate via a side channel. For L1 we
    // surface them via the log; the driver renders both. M2-L9 swaps in
    // proper Diagnostic formatting.
    for (const auto& d : ctx.diagnostics) {
        std::string sev = (d.severity == wvmcc::Diagnostic::Severity::Error)
                              ? "error"
                              : "warning";
        r.log.push_back(sev + ": " + d.message);
    }
    return r;
}

} // namespace wvmcc::link
