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

} // namespace

void resolveImports(LinkContext& ctx) {
    auto& m = ctx.output;

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
