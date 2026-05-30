#include "Linker.hpp"
#include "LinkContext.hpp"
#include "ModuleMerge.hpp"
#include "Crt0Synth.hpp"
#include "SymbolResolver.hpp"
#include "LinkDiagnostics.hpp"
#include "RelocApply.hpp"
#include "DeadCodeEliminator.hpp"
#include "MapWriter.hpp"

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
// remap + import/export concatenation). Archive inputs error here until
// M2-L4 lands.
void phaseMerge(LinkContext& ctx) {
    ctx.note("phase: merge");
    if (ctx.inputs.empty()) {
        ctx.error("link: no inputs");
        return;
    }
    for (const auto& in : ctx.inputs) {
        if (auto* mm = std::get_if<LinkInput::InMemoryModule>(&in.source)) {
            merge::mergeOne(ctx, mm->module, mm->origin);
            if (ctx.hasErrors()) return;
            std::ostringstream ss;
            ss << "  merged " << mm->origin
               << " (" << mm->module.funcs.size() << " func defs, "
               << mm->module.types.size() << " types)";
            ctx.note(ss.str());
        } else {
            ctx.error("link: archive inputs not yet supported (M2-L4 deferred until libc.a exists)");
            return;
        }
    }
}

// M2-L7: rewrite per-TU funcref element segments into the merged shared
// __indirect_function_table layout. Stub for L1.
void phaseIndirectTableMerge(LinkContext& ctx) {
    ctx.note("phase: indirect-table-merge (stub)");
}

// M2-L8: rebase data segments + apply each TU's reloc.CODE entries.
void phaseRelocApply(LinkContext& ctx) {
    ctx.note("phase: reloc-apply");
    reloc::applyRelocations(ctx);
}

// M2-L6: synthesize crt0 — drop env.__* imports, replace with local defs
// at the same index positions, prepend sys_proc imports (shifting func
// indices by +4), and emit the start wrapper.
void phaseCrt0(LinkContext& ctx) {
    ctx.note("phase: crt0-synth");
    crt0::synthesize(ctx);
}

// M2-L3: resolve cross-module imports against the merged + crt0 exports.
void phaseResolveImports(LinkContext& ctx) {
    ctx.note("phase: import-resolution");
    resolve::resolveImports(ctx);
}

// M2-L5: mark-and-sweep DCE across the merged output.
void phaseDeadCodeElim(LinkContext& ctx) {
    ctx.note("phase: dce");
    dce::eliminate(ctx);
}

// M2-L9: report unresolved imports outside the host-runtime allow-list.
void phaseDiagnostics(LinkContext& ctx) {
    ctx.note("phase: diagnostics");
    diag::emitUnresolvedDiagnostics(ctx);
}

// M2-L10: write --map=<path> output.
void phaseMapOutput(LinkContext& ctx) {
    if (ctx.opts.map_path.empty()) return;
    ctx.note("phase: map-output");
    map::writeMap(ctx);
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
