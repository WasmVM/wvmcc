#include "DeadCodeEliminator.hpp"

#include <queue>
#include <sstream>
#include <unordered_set>
#include <variant>

namespace wvmcc::link::dce {

namespace {

WasmVM::index_t funcImportCount(const WasmVM::WasmModule& m) {
    WasmVM::index_t c = 0;
    for (const auto& imp : m.imports) {
        if (std::holds_alternative<WasmVM::index_t>(imp.desc)) ++c;
    }
    return c;
}

void visitConstInstrFuncRefs(const WasmVM::ConstInstr& c,
                             std::unordered_set<WasmVM::index_t>& sink) {
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::Ref_func>) {
            sink.insert(v.index);
        }
    }, c);
}

void rewriteConstInstr(WasmVM::ConstInstr& c,
                       const std::vector<WasmVM::index_t>& remap) {
    std::visit([&](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::Ref_func>) {
            if (v.index < remap.size()) v.index = remap[v.index];
        }
    }, c);
}

} // namespace

void eliminate(LinkContext& ctx) {
    auto& m = ctx.output;
    if (m.funcs.empty()) return;

    namespace Op = WasmVM::Opcode;
    const WasmVM::index_t imports = funcImportCount(m);
    const WasmVM::index_t totalFuncs = imports + (WasmVM::index_t)m.funcs.size();

    // Conservative: collect every funcidx that ever appears as a Ref_func
    // anywhere in the module. Any of those can be invoked via call_ref, so
    // we mark them reachable up-front.
    std::unordered_set<WasmVM::index_t> refFunced;
    for (const auto& f : m.funcs) {
        for (const auto& instr : f.body) {
            if (instr.opcode == Op::Ref_func) {
                if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
                    refFunced.insert(oi->index);
                }
            }
        }
    }
    for (const auto& e : m.elems) {
        for (const auto& entry : e.elemlist) visitConstInstrFuncRefs(entry, refFunced);
    }
    for (const auto& g : m.globals) {
        visitConstInstrFuncRefs(g.init, refFunced);
    }

    // Seed.
    std::unordered_set<WasmVM::index_t> mark;
    std::queue<WasmVM::index_t> work;
    auto seed = [&](WasmVM::index_t idx) {
        if (idx >= totalFuncs) return;
        if (mark.insert(idx).second) work.push(idx);
    };

    if (m.start.has_value()) {
        // Final executable: the entry point and its closure are the only live
        // code. Exported libc functions that nothing reachable calls (e.g. an
        // unused qsort pulled transitively) are dead and get stripped.
        seed(*m.start);
    } else {
        // Relocatable object (no crt0): every exported function is a public
        // entry the next link stage may reference, so all are roots.
        for (const auto& ex : m.exports) {
            if (ex.desc == WasmVM::WasmExport::DescType::func) seed(ex.index);
        }
    }
    for (auto idx : refFunced) seed(idx);
    // All imports are always considered reachable (they're contracts with
    // the runtime / linker, not dead code).
    for (WasmVM::index_t i = 0; i < imports; ++i) seed(i);

    // Propagate.
    while (!work.empty()) {
        WasmVM::index_t cur = work.front();
        work.pop();
        if (cur < imports) continue; // imports have no body to walk
        const auto& f = m.funcs[cur - imports];
        for (const auto& instr : f.body) {
            switch (instr.opcode) {
                case Op::Call:
                case Op::Return_call:
                case Op::Ref_func: {
                    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
                        seed(oi->index);
                    }
                    break;
                }
                default: break;
            }
        }
    }

    // Sweep: build the new funcs list with only marked defined funcs.
    WasmVM::index_t before = (WasmVM::index_t)m.funcs.size();
    std::vector<WasmVM::WasmFunc> kept;
    std::vector<WasmVM::index_t> remap(totalFuncs);
    for (WasmVM::index_t i = 0; i < imports; ++i) remap[i] = i; // imports unchanged
    WasmVM::index_t newDefIdx = imports;
    for (WasmVM::index_t i = 0; i < before; ++i) {
        WasmVM::index_t oldIdx = imports + i;
        if (mark.count(oldIdx)) {
            kept.push_back(std::move(m.funcs[i]));
            remap[oldIdx] = newDefIdx++;
        } else {
            remap[oldIdx] = (WasmVM::index_t)-1; // dead
        }
    }
    m.funcs = std::move(kept);

    WasmVM::index_t after = (WasmVM::index_t)m.funcs.size();
    if (after == before) return; // nothing changed

    // Rewrite every funcidx reference in the surviving module.
    auto remapIdx = [&](WasmVM::index_t old) -> WasmVM::index_t {
        if (old < remap.size() && remap[old] != (WasmVM::index_t)-1) return remap[old];
        return old; // shouldn't happen for marked-reachable code
    };
    for (auto& f : m.funcs) {
        for (auto& instr : f.body) {
            switch (instr.opcode) {
                case Op::Call:
                case Op::Return_call:
                case Op::Ref_func: {
                    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
                        oi->index = remapIdx(oi->index);
                    }
                    break;
                }
                default: break;
            }
        }
    }
    // Exports: drop entries that point at removed functions.
    {
        std::vector<WasmVM::WasmExport> keptEx;
        for (auto& ex : m.exports) {
            if (ex.desc == WasmVM::WasmExport::DescType::func) {
                if (ex.index < remap.size() && remap[ex.index] == (WasmVM::index_t)-1) {
                    continue; // exported a dead function; drop
                }
                ex.index = remapIdx(ex.index);
            }
            keptEx.push_back(std::move(ex));
        }
        m.exports = std::move(keptEx);
    }
    // Elements: rewrite Ref_func entries. Dead entries (referencing
    // removed funcs) replace with Ref_null funcref — but in practice DCE
    // only removes a function if no Ref_func references it, so this case
    // doesn't trigger.
    for (auto& e : m.elems) {
        for (auto& entry : e.elemlist) rewriteConstInstr(entry, remap);
    }
    for (auto& g : m.globals) {
        rewriteConstInstr(g.init, remap);
    }
    if (m.start.has_value()) {
        m.start = remapIdx(*m.start);
    }

    std::ostringstream ss;
    ss << "  dce: " << before << " → " << after << " defined funcs ("
       << (before - after) << " removed)";
    ctx.note(ss.str());
}

} // namespace wvmcc::link::dce
