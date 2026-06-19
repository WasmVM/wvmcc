#include "SymbolResolver.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace wvmcc::link::resolve {

namespace {

// Host runtime imports we always leave alone — they're satisfied at
// instantiation time by wasmvm's host glue, not by any linked TU.
const std::unordered_set<std::string> kHostModules = {"sys_proc", "sys_fs"};

void rewriteConstInstr(WasmVM::ConstInstr& c,
                       const std::vector<WasmVM::index_t>& funcRemap) {
    std::visit([&](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::Ref_func>) {
            if (v.index < funcRemap.size()) v.index = funcRemap[v.index];
        }
    }, c);
}

// Resolve cross-module GLOBAL imports against the merged exports by name —
// the data-symbol analogue of function resolution. A defining TU exports an
// `i64` address-global (e.g. `errno`); referencing TUs import a global of the
// same name and `global.get` it. Here we drop each resolved global import and
// rewrite every global-index reference that pointed at it to the defining
// global's index. Imported globals occupy the low slots of the global index
// space, so removing some shifts the defined globals down by the resolved
// count — exactly mirroring resolveImports' function handling.
//
// Single-definition is the libc case (one `int errno;`, many `extern`s); no
// address collision arises because only one TU defines the object. (Relocating
// multiple defining TUs' data so their addresses don't overlap is a separate,
// pre-existing concern handled by RelocApply.)
void resolveGlobalImports(LinkContext& ctx) {
    auto& m = ctx.output;

    std::unordered_map<std::string, WasmVM::index_t> globalExport; // name → global idx
    for (const auto& ex : m.exports) {
        if (ex.desc == WasmVM::WasmExport::DescType::global)
            globalExport[ex.name] = ex.index;
    }

    std::vector<WasmVM::index_t> globalRemap; // old global-import idx → new global idx
    std::vector<WasmVM::WasmImport> kept;
    WasmVM::index_t newGlobalImportIdx = 0;
    int resolvedCount = 0;
    std::vector<bool> isResolved;
    std::vector<WasmVM::index_t> oldTarget; // valid only when isResolved[i]

    for (const auto& imp : m.imports) {
        if (std::holds_alternative<WasmVM::GlobalType>(imp.desc)) {
            auto it = globalExport.find(imp.name);
            const bool isHost = kHostModules.count(imp.module) > 0;
            const bool resolve = !isHost && it != globalExport.end();
            if (resolve) {
                isResolved.push_back(true);
                oldTarget.push_back(it->second);
                globalRemap.push_back(0); // fixed below
                ++resolvedCount;
            } else {
                isResolved.push_back(false);
                oldTarget.push_back(0);
                globalRemap.push_back(newGlobalImportIdx++);
                kept.push_back(imp);
            }
        } else {
            kept.push_back(imp);
        }
    }

    if (resolvedCount == 0) return; // nothing to do — leave imports/index space as-is

    const WasmVM::index_t oldGlobalImportCount = (WasmVM::index_t)globalRemap.size();
    const WasmVM::index_t newGlobalImportCount = newGlobalImportIdx;
    auto remapDefinedGlobal = [&](WasmVM::index_t old) -> WasmVM::index_t {
        return newGlobalImportCount + (old - oldGlobalImportCount);
    };
    for (size_t i = 0; i < globalRemap.size(); ++i) {
        if (isResolved[i]) globalRemap[i] = remapDefinedGlobal(oldTarget[i]);
    }
    auto remapGlobal = [&](WasmVM::index_t old) -> WasmVM::index_t {
        if (old < globalRemap.size()) return globalRemap[old];
        return remapDefinedGlobal(old);
    };

    namespace Op = WasmVM::Opcode;
    // Rewrite every global-index reference: global.get / global.set in code...
    for (auto& f : m.funcs) {
        for (auto& instr : f.body) {
            if (instr.opcode == Op::Global_get || instr.opcode == Op::Global_set) {
                if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
                    oi->index = remapGlobal(oi->index);
                }
            }
        }
    }
    // ...global exports...
    for (auto& ex : m.exports) {
        if (ex.desc == WasmVM::WasmExport::DescType::global) {
            ex.index = remapGlobal(ex.index);
        }
    }
    // ...and global.get inside const-expr initializers (global inits).
    for (auto& g : m.globals) {
        std::visit([&](auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, WasmVM::Instr::Global_get>) {
                v.index = remapGlobal(v.index);
            }
        }, g.init);
    }

    m.imports = std::move(kept);
}

// Imports that are runtime contracts (satisfied by the host at instantiation
// or by crt0), never user externs — mirrors the diagnostics allow-list. These
// are kept even when nothing in the module references them.
const std::unordered_set<std::string> kEnvRuntimeState = {
    "__linear_memory", "__stack_memory", "__stack_pointer",
    "__heap_base", "__indirect_function_table",
};

bool isRuntimeImport(const WasmVM::WasmImport& imp) {
    if (kHostModules.count(imp.module) > 0) return true;
    if (imp.module == "env" && kEnvRuntimeState.count(imp.name) > 0) return true;
    return false;
}

void collectConstInstrFuncRefs(const WasmVM::ConstInstr& c,
                               std::unordered_set<WasmVM::index_t>& sink) {
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::Ref_func>) sink.insert(v.index);
    }, c);
}

// Drop function imports that nothing in the (post-DCE) module references. A
// declared-but-unused `extern` introduces no link requirement (C 6.9p5), so an
// unresolved import with zero references must be removed rather than reported —
// otherwise the emitted module would import a symbol the host can't satisfy and
// fail to instantiate. Runtime-contract imports are always kept. Reindexing
// mirrors resolveImports: removing low-index function imports shifts later
// function indices down.
void pruneUnreferencedFunctionImports(LinkContext& ctx) {
    auto& m = ctx.output;
    namespace Op = WasmVM::Opcode;

    std::unordered_set<WasmVM::index_t> referenced;
    for (const auto& f : m.funcs) {
        for (const auto& instr : f.body) {
            if (instr.opcode == Op::Call || instr.opcode == Op::Return_call
                || instr.opcode == Op::Ref_func) {
                if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm))
                    referenced.insert(oi->index);
            }
        }
    }
    for (const auto& e : m.elems)
        for (const auto& entry : e.elemlist) collectConstInstrFuncRefs(entry, referenced);
    for (const auto& g : m.globals) collectConstInstrFuncRefs(g.init, referenced);
    for (const auto& ex : m.exports)
        if (ex.desc == WasmVM::WasmExport::DescType::func) referenced.insert(ex.index);
    if (m.start.has_value()) referenced.insert(*m.start);

    std::vector<WasmVM::index_t> funcRemap; // old func-import idx → new funcidx
    std::vector<WasmVM::WasmImport> kept;
    WasmVM::index_t newFuncImportIdx = 0;
    WasmVM::index_t funcImportIdx = 0;
    int droppedCount = 0;
    for (const auto& imp : m.imports) {
        if (std::holds_alternative<WasmVM::index_t>(imp.desc)) {
            WasmVM::index_t thisIdx = funcImportIdx++;
            const bool keep = isRuntimeImport(imp) || referenced.count(thisIdx) > 0;
            if (keep) {
                funcRemap.push_back(newFuncImportIdx++);
                kept.push_back(imp);
            } else {
                funcRemap.push_back((WasmVM::index_t)-1); // dropped; never referenced
                ++droppedCount;
            }
        } else {
            kept.push_back(imp);
        }
    }
    if (droppedCount == 0) return;

    const WasmVM::index_t oldFuncImportCount = (WasmVM::index_t)funcRemap.size();
    const WasmVM::index_t newFuncImportCount = newFuncImportIdx;
    auto remapFunc = [&](WasmVM::index_t old) -> WasmVM::index_t {
        if (old < funcRemap.size()) return funcRemap[old];
        return newFuncImportCount + (old - oldFuncImportCount);
    };

    for (auto& f : m.funcs) {
        for (auto& instr : f.body) {
            switch (instr.opcode) {
                case Op::Call:
                case Op::Return_call:
                case Op::Ref_func:
                    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm))
                        oi->index = remapFunc(oi->index);
                    break;
                default: break;
            }
        }
    }
    for (auto& ex : m.exports)
        if (ex.desc == WasmVM::WasmExport::DescType::func) ex.index = remapFunc(ex.index);
    {
        std::vector<WasmVM::index_t> fullRemap;
        WasmVM::index_t totalOldFuncs = oldFuncImportCount + (WasmVM::index_t)m.funcs.size();
        fullRemap.reserve(totalOldFuncs);
        for (WasmVM::index_t i = 0; i < totalOldFuncs; ++i) fullRemap.push_back(remapFunc(i));
        for (auto& e : m.elems)
            for (auto& entry : e.elemlist) rewriteConstInstr(entry, fullRemap);
        for (auto& g : m.globals) rewriteConstInstr(g.init, fullRemap);
    }
    if (m.start.has_value()) m.start = remapFunc(*m.start);

    m.imports = std::move(kept);
}

// Global-import analogue of pruneUnreferencedFunctionImports.
void pruneUnreferencedGlobalImports(LinkContext& ctx) {
    auto& m = ctx.output;
    namespace Op = WasmVM::Opcode;

    std::unordered_set<WasmVM::index_t> referenced;
    for (const auto& f : m.funcs) {
        for (const auto& instr : f.body) {
            if (instr.opcode == Op::Global_get || instr.opcode == Op::Global_set) {
                if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm))
                    referenced.insert(oi->index);
            }
        }
    }
    for (const auto& ex : m.exports)
        if (ex.desc == WasmVM::WasmExport::DescType::global) referenced.insert(ex.index);
    for (const auto& g : m.globals) {
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, WasmVM::Instr::Global_get>) referenced.insert(v.index);
        }, g.init);
    }

    std::vector<WasmVM::index_t> globalRemap;
    std::vector<WasmVM::WasmImport> kept;
    WasmVM::index_t newGlobalImportIdx = 0;
    WasmVM::index_t globalImportIdx = 0;
    int droppedCount = 0;
    for (const auto& imp : m.imports) {
        if (std::holds_alternative<WasmVM::GlobalType>(imp.desc)) {
            WasmVM::index_t thisIdx = globalImportIdx++;
            const bool keep = isRuntimeImport(imp) || referenced.count(thisIdx) > 0;
            if (keep) {
                globalRemap.push_back(newGlobalImportIdx++);
                kept.push_back(imp);
            } else {
                globalRemap.push_back((WasmVM::index_t)-1);
                ++droppedCount;
            }
        } else {
            kept.push_back(imp);
        }
    }
    if (droppedCount == 0) return;

    const WasmVM::index_t oldGlobalImportCount = (WasmVM::index_t)globalRemap.size();
    const WasmVM::index_t newGlobalImportCount = newGlobalImportIdx;
    auto remapGlobal = [&](WasmVM::index_t old) -> WasmVM::index_t {
        if (old < globalRemap.size()) return globalRemap[old];
        return newGlobalImportCount + (old - oldGlobalImportCount);
    };

    for (auto& f : m.funcs) {
        for (auto& instr : f.body) {
            if (instr.opcode == Op::Global_get || instr.opcode == Op::Global_set) {
                if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm))
                    oi->index = remapGlobal(oi->index);
            }
        }
    }
    for (auto& ex : m.exports)
        if (ex.desc == WasmVM::WasmExport::DescType::global) ex.index = remapGlobal(ex.index);
    for (auto& g : m.globals) {
        std::visit([&](auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, WasmVM::Instr::Global_get>) v.index = remapGlobal(v.index);
        }, g.init);
    }

    m.imports = std::move(kept);
}

} // namespace

void pruneUnreferencedImports(LinkContext& ctx) {
    // Independent index spaces; order doesn't matter.
    pruneUnreferencedGlobalImports(ctx);
    pruneUnreferencedFunctionImports(ctx);
}

void resolveImports(LinkContext& ctx) {
    auto& m = ctx.output;

    // Resolve cross-module data globals first (independent global index space;
    // order vs. function resolution doesn't matter). Done before the function
    // logic's early-return so it runs even when no function imports resolve.
    resolveGlobalImports(ctx);

    // Build a name → (kind, index) export table.
    struct ExportEntry {
        WasmVM::index_t index;
        WasmVM::WasmExport::DescType desc;
    };
    std::unordered_map<std::string, ExportEntry> exportByName;
    for (const auto& ex : m.exports) {
        exportByName[ex.name] = {ex.index, ex.desc};
    }

    // Walk imports left-to-right, building per-input-funcidx remap and the
    // kept-imports list.
    std::vector<WasmVM::index_t> funcRemap; // old func-import idx → new funcidx
    std::vector<WasmVM::WasmImport> kept;
    WasmVM::index_t newFuncImportIdx = 0;
    int resolvedCount = 0;

    // Track each resolved import's *old* target funcidx; we translate it
    // to the new index space below once we know how many imports were
    // dropped.
    std::vector<bool> isResolved;
    std::vector<WasmVM::index_t> oldTarget; // valid only when isResolved[i]
    for (const auto& imp : m.imports) {
        if (std::holds_alternative<WasmVM::index_t>(imp.desc)) {
            auto it = exportByName.find(imp.name);
            const bool isHost = kHostModules.count(imp.module) > 0;
            const bool resolve = !isHost && it != exportByName.end() &&
                                 it->second.desc == WasmVM::WasmExport::DescType::func;
            if (resolve) {
                isResolved.push_back(true);
                oldTarget.push_back(it->second.index);
                funcRemap.push_back(0); // placeholder, fixed below
                ++resolvedCount;
            } else {
                isResolved.push_back(false);
                oldTarget.push_back(0);
                funcRemap.push_back(newFuncImportIdx++);
                kept.push_back(imp);
            }
        } else {
            kept.push_back(imp);
        }
    }

    if (resolvedCount == 0) return; // nothing to do — keep imports + index space as-is

    // Function indices for already-defined funcs (the part of the index
    // space after function imports) shift down by `resolvedCount` because
    // some function imports were removed from the front of the index
    // space.
    const WasmVM::index_t oldFuncImportCount = (WasmVM::index_t)funcRemap.size();
    const WasmVM::index_t newFuncImportCount = newFuncImportIdx;

    auto remapDefinedFunc = [&](WasmVM::index_t old) -> WasmVM::index_t {
        return newFuncImportCount + (old - oldFuncImportCount);
    };

    // Now that the layout is known, fill in resolved imports' targets by
    // remapping their old export target (always a defined-func index).
    for (size_t i = 0; i < funcRemap.size(); ++i) {
        if (isResolved[i]) {
            funcRemap[i] = remapDefinedFunc(oldTarget[i]);
        }
    }

    auto remapFunc = [&](WasmVM::index_t old) -> WasmVM::index_t {
        if (old < funcRemap.size()) return funcRemap[old];
        return remapDefinedFunc(old);
    };

    namespace Op = WasmVM::Opcode;
    // Rewrite every function-index reference in the module.
    for (auto& f : m.funcs) {
        for (auto& instr : f.body) {
            switch (instr.opcode) {
                case Op::Call:
                case Op::Return_call:
                case Op::Ref_func: {
                    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
                        oi->index = remapFunc(oi->index);
                    }
                    break;
                }
                default: break;
            }
        }
    }
    for (auto& ex : m.exports) {
        if (ex.desc == WasmVM::WasmExport::DescType::func) {
            ex.index = remapFunc(ex.index);
        }
    }
    {
        // Build a one-shot funcRemap copy that covers up through defined
        // funcs so ConstInstr (which only sees Ref_func indices in funcRemap)
        // can use the lambda's behavior.
        std::vector<WasmVM::index_t> fullRemap;
        WasmVM::index_t totalOldFuncs = oldFuncImportCount + (WasmVM::index_t)m.funcs.size();
        fullRemap.reserve(totalOldFuncs);
        for (WasmVM::index_t i = 0; i < totalOldFuncs; ++i) fullRemap.push_back(remapFunc(i));
        for (auto& e : m.elems) {
            for (auto& entry : e.elemlist) rewriteConstInstr(entry, fullRemap);
        }
        for (auto& g : m.globals) {
            rewriteConstInstr(g.init, fullRemap);
        }
    }
    if (m.start.has_value()) {
        m.start = remapFunc(*m.start);
    }

    m.imports = std::move(kept);
}

} // namespace wvmcc::link::resolve
