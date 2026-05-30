#include "Linker.hpp"
#include "LinkContext.hpp"
#include "ModuleMerge.hpp"
#include "Crt0Synth.hpp"
#include "SymbolResolver.hpp"
#include "LinkDiagnostics.hpp"
#include "RelocApply.hpp"
#include "DeadCodeEliminator.hpp"
#include "MapWriter.hpp"
#include "ArchiveReader.hpp"

#include <memory>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <variant>

namespace wvmcc::link {

// ---------------------------------------------------------------------------
// Phase stubs. Each lands in its own M2-L* issue; M2-L1 just wires the
// orchestration so the driver has a runnable plumbing skeleton.
// ---------------------------------------------------------------------------

namespace {

// Runtime imports that no archive member ever satisfies: host glue
// (sys_proc/sys_fs, bound at instantiation) and the crt0-provided env.__*
// runtime state (memories + stack/heap globals, materialized by phaseCrt0).
// Excluding them from the lazy-pull worklist avoids scanning archives for
// names that can never match an export.
bool isRuntimeProvided(const WasmVM::WasmImport& imp) {
    if (imp.module == "sys_proc" || imp.module == "sys_fs") return true;
    if (imp.module == "env" && imp.name.rfind("__", 0) == 0) return true;
    return false;
}

// Names imported by the merged output that no merged export satisfies yet —
// the set an archive member must export to be worth pulling.
std::unordered_set<std::string> unresolvedNames(const WasmVM::WasmModule& m) {
    std::unordered_set<std::string> exports;
    for (const auto& ex : m.exports) exports.insert(ex.name);
    std::unordered_set<std::string> need;
    for (const auto& imp : m.imports) {
        if (isRuntimeProvided(imp)) continue;
        if (!exports.count(imp.name)) need.insert(imp.name);
    }
    return need;
}

// Does this candidate module export any name the output still needs?
bool memberSatisfies(const WasmVM::WasmModule& cand,
                     const std::unordered_set<std::string>& need) {
    for (const auto& ex : cand.exports) {
        if (need.count(ex.name)) return true;
    }
    return false;
}

// M2-L4: lazy archive pulls. With every user TU already merged, repeatedly
// scan the archives and pull any member that satisfies a currently-unresolved
// import, until a full pass adds nothing. Standard Unix linker semantics:
// only the TUs actually needed end up in the binary.
void lazyPullArchives(LinkContext& ctx,
                      std::vector<std::unique_ptr<ArchiveReader>>& archives) {
    if (archives.empty()) return;

    // Track which (archive, member) pairs were pulled so we never merge twice.
    std::vector<std::vector<char>> pulled(archives.size());
    for (size_t a = 0; a < archives.size(); ++a)
        pulled[a].assign(archives[a]->memberCount(), 0);

    auto need = unresolvedNames(ctx.output);

    bool progress = true;
    while (progress && !need.empty()) {
        progress = false;
        for (size_t a = 0; a < archives.size() && !need.empty(); ++a) {
            ArchiveReader& ar = *archives[a];
            for (size_t i = 0; i < ar.memberCount(); ++i) {
                if (pulled[a][i]) continue;
                const WasmVM::WasmModule* cand = ar.module(i);
                if (!cand) { ctx.error("link: " + ar.error()); return; }
                if (!memberSatisfies(*cand, need)) continue;

                merge::mergeOne(ctx, *cand, ar.memberName(i));
                if (ctx.hasErrors()) return;
                pulled[a][i] = 1;
                progress = true;
                std::ostringstream ss;
                ss << "  pulled " << ar.memberName(i) << " from " << ar.path()
                   << " (" << cand->funcs.size() << " func defs)";
                ctx.note(ss.str());

                // Pulling a member adds its own unresolved imports; recompute
                // and restart the scan from the first archive (the issue's
                // fixpoint: a later member may now be needed by an earlier
                // archive).
                need = unresolvedNames(ctx.output);
                i = (size_t)-1; // ++i restarts this archive at 0
            }
        }
    }
}

// M2-L2 + M2-L4: merge each in-memory user TU into ctx.output, then lazily
// pull whichever archive members are needed to satisfy unresolved imports.
void phaseMerge(LinkContext& ctx) {
    ctx.note("phase: merge");
    if (ctx.inputs.empty()) {
        ctx.error("link: no inputs");
        return;
    }
    std::vector<std::unique_ptr<ArchiveReader>> archives;
    for (const auto& in : ctx.inputs) {
        if (auto* mm = std::get_if<LinkInput::InMemoryModule>(&in.source)) {
            merge::mergeOne(ctx, mm->module, mm->origin);
            if (ctx.hasErrors()) return;
            std::ostringstream ss;
            ss << "  merged " << mm->origin
               << " (" << mm->module.funcs.size() << " func defs, "
               << mm->module.types.size() << " types)";
            ctx.note(ss.str());
        } else if (auto* ap = std::get_if<LinkInput::ArchivePath>(&in.source)) {
            auto ar = std::make_unique<ArchiveReader>(ap->path);
            if (!ar->ok()) {
                ctx.error("link: " + ar->error());
                return;
            }
            archives.push_back(std::move(ar));
        }
    }

    lazyPullArchives(ctx, archives);
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
        // Resolve cross-module imports BEFORE crt0. This drops every resolvable
        // function/global import (puts→puts, errno→errno, …) so that by the
        // time crt0 runs, the only remaining global imports are the runtime
        // state it replaces (env.__stack_pointer / env.__heap_base) — letting
        // crt0 substitute them at global indices 0/1 without disturbing the
        // index space that function bodies reference.
        phaseResolveImports(ctx);
        phaseCrt0(ctx);
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
