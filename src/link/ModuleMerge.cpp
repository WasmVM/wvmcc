#include <cstdint>
#include "ModuleMerge.hpp"

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace wvmcc::link::merge {

namespace {

// Hash key for (module, name) import dedup.
struct ImportKey {
    std::string module;
    std::string name;
    bool operator==(const ImportKey& o) const {
        return module == o.module && name == o.name;
    }
};
struct ImportKeyHash {
    size_t operator()(const ImportKey& k) const {
        return std::hash<std::string>{}(k.module) ^
               (std::hash<std::string>{}(k.name) << 1);
    }
};

// Equality for FuncType (used for dedup).
bool sameFuncType(const WasmVM::FuncType& a, const WasmVM::FuncType& b) {
    return a.params == b.params && a.results == b.results;
}

WasmVM::index_t internType(WasmVM::WasmModule& out, const WasmVM::FuncType& t) {
    for (WasmVM::index_t i = 0; i < (WasmVM::index_t)out.types.size(); ++i) {
        if (sameFuncType(out.types[i], t)) return i;
    }
    out.types.push_back(t);
    return (WasmVM::index_t)(out.types.size() - 1);
}

// State tracked across calls to mergeOne — lets us dedup imports against
// previously merged modules.
struct DedupState {
    // (module, name) → output-index in the appropriate index space.
    std::unordered_map<ImportKey, WasmVM::index_t, ImportKeyHash> funcImports;
    std::unordered_map<ImportKey, WasmVM::index_t, ImportKeyHash> globalImports;
    std::unordered_map<ImportKey, WasmVM::index_t, ImportKeyHash> memImports;
    std::unordered_map<ImportKey, WasmVM::index_t, ImportKeyHash> tableImports;

    // Running counts within the output's index spaces (imports first, then
    // defs). These let us assign new indices as imports/defs are appended.
    WasmVM::index_t funcImportCount   = 0;  // number of func imports so far
    WasmVM::index_t globalImportCount = 0;
    WasmVM::index_t memImportCount    = 0;
    WasmVM::index_t tableImportCount  = 0;

    // Whether any merged module set the start section.
    bool startSet = false;

    // M2-L8: running top of placed mem[0] data across merged TUs (0 = nothing
    // placed yet). Each TU's data is rebased to start at/after this, so per-TU
    // data segments (all emitted starting at offset 8) don't collide.
    uint64_t dataTop = 0;

    // #79: running count of funcref-table slots placed across merged inputs.
    // Each input's table entries are appended after this, its element-segment
    // offset and its function-pointer i64.const slots shifted to match.
    uint64_t slotTop = 0;
};

// #79: function-pointer tag layout (must match FunctionCodegen). A function
// pointer is `(kFuncPtrTag | slot)`; rebasing shifts only the slot bits.
static constexpr int64_t kFuncPtrTag = (int64_t)0xF << 60;
static constexpr int64_t kFuncPtrSlotMask = ~((int64_t)0xF << 60);

// Singleton-per-link tracking. We stash this on LinkContext via a side
// channel below.
DedupState& dedupFor(LinkContext& ctx) {
    static thread_local DedupState s;
    // Reset when the output module is empty (start of a new link).
    if (ctx.output.imports.empty() && ctx.output.types.empty() &&
        ctx.output.funcs.empty() && ctx.output.exports.empty()) {
        s = DedupState{};
    }
    return s;
}

void remapConstInstr(WasmVM::ConstInstr& c, const Remap& r) {
    std::visit([&](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::Global_get>) {
            if (v.index < r.global.size()) v.index = r.global[v.index];
        } else if constexpr (std::is_same_v<T, WasmVM::Instr::Ref_func>) {
            if (v.index < r.func.size()) v.index = r.func[v.index];
        }
        // I32/I64/F32/F64_const and Ref_null carry no index.
    }, c);
}

// Read a data segment's active offset (the i64/i32 const in mode.offset).
// Returns 0 for passive segments or non-const offsets.
uint64_t dataSegOffset(const WasmVM::WasmData& d) {
    if (!d.mode.offset.has_value()) return 0;
    uint64_t out = 0;
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WasmVM::Instr::I64_const> ||
                      std::is_same_v<T, WasmVM::Instr::I32_const>) {
            out = (uint64_t)v.value;
        }
    }, *d.mode.offset);
    return out;
}

// When a freshly-merged module appends new function/global imports, every
// already-merged *defined* func/global shifts up in the shared index space
// (imports precede definitions). Rewrite every reference in the existing
// output that targets a definition (index >= the pre-merge import count) by
// the number of newly-added imports, so prior modules keep pointing at the
// same definitions. Must run after this module's import loop but before any
// of its own funcs/globals/exports are appended.
void shiftExistingDefRefs(WasmVM::WasmModule& out,
                          WasmVM::index_t funcThreshold, WasmVM::index_t funcDelta,
                          WasmVM::index_t globalThreshold, WasmVM::index_t globalDelta) {
    namespace Op = WasmVM::Opcode;
    if (funcDelta == 0 && globalDelta == 0) return;
    auto bumpFunc = [&](WasmVM::index_t& idx) {
        if (funcDelta && idx >= funcThreshold) idx += funcDelta;
    };
    auto bumpGlobal = [&](WasmVM::index_t& idx) {
        if (globalDelta && idx >= globalThreshold) idx += globalDelta;
    };
    auto bumpConst = [&](WasmVM::ConstInstr& c) {
        std::visit([&](auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, WasmVM::Instr::Ref_func>) bumpFunc(v.index);
            else if constexpr (std::is_same_v<T, WasmVM::Instr::Global_get>) bumpGlobal(v.index);
        }, c);
    };
    for (auto& f : out.funcs) {
        for (auto& instr : f.body) {
            switch (instr.opcode) {
                case Op::Call:
                case Op::Return_call:
                case Op::Ref_func:
                    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm))
                        bumpFunc(oi->index);
                    break;
                case Op::Global_get:
                case Op::Global_set:
                    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm))
                        bumpGlobal(oi->index);
                    break;
                default: break;
            }
        }
    }
    for (auto& ex : out.exports) {
        if (ex.desc == WasmVM::WasmExport::DescType::func) bumpFunc(ex.index);
        else if (ex.desc == WasmVM::WasmExport::DescType::global) bumpGlobal(ex.index);
    }
    for (auto& e : out.elems)
        for (auto& entry : e.elemlist) bumpConst(entry);
    for (auto& g : out.globals) bumpConst(g.init);
    if (out.start.has_value()) bumpFunc(*out.start);
}

} // anonymous namespace

void remapInstr(WasmVM::WasmInstr& instr, const Remap& r) {
    namespace Op = WasmVM::Opcode;

    auto remapOneIdx = [&](const std::vector<WasmVM::index_t>& tbl) {
        if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
            if (oi->index < tbl.size()) oi->index = tbl[oi->index];
        }
    };

    switch (instr.opcode) {
        // Function index space.
        case Op::Call:
        case Op::Return_call:
        case Op::Ref_func:
            remapOneIdx(r.func);
            break;

        // Global index space.
        case Op::Global_get:
        case Op::Global_set:
            remapOneIdx(r.global);
            break;

        // Table index space (single-index variants).
        case Op::Table_get:
        case Op::Table_set:
        case Op::Table_size:
        case Op::Table_grow:
        case Op::Table_fill:
            remapOneIdx(r.table);
            break;

        // call_indirect / return_call_indirect: (tableidx, typeidx).
        case Op::Call_indirect:
        case Op::Return_call_indirect: {
            if (auto* ti = std::get_if<WasmVM::WasmInstr::TwoIdx>(&instr.imm)) {
                if (ti->a < r.table.size()) ti->a = r.table[ti->a];
                if (ti->b < r.type.size())  ti->b = r.type[ti->b];
            }
            break;
        }

        // call_ref / return_call_ref: a single *type* index.
        case Op::Call_ref:
        case Op::Return_call_ref:
            remapOneIdx(r.type);
            break;

        // table.copy: (dst_tableidx, src_tableidx).
        case Op::Table_copy: {
            if (auto* ti = std::get_if<WasmVM::WasmInstr::TwoIdx>(&instr.imm)) {
                if (ti->a < r.table.size()) ti->a = r.table[ti->a];
                if (ti->b < r.table.size()) ti->b = r.table[ti->b];
            }
            break;
        }

        // table.init: (tableidx, elemidx) — leave elemidx alone (not handled
        // by M2-L2; deferred to M2-L7).
        case Op::Table_init: {
            if (auto* ti = std::get_if<WasmVM::WasmInstr::TwoIdx>(&instr.imm)) {
                if (ti->a < r.table.size()) ti->a = r.table[ti->a];
            }
            break;
        }

        // memory.* single-index forms: memidx.
        case Op::Memory_size:
        case Op::Memory_grow:
        case Op::Memory_fill:
            remapOneIdx(r.mem);
            break;

        // memory.init: (memidx, dataidx).
        case Op::Memory_init: {
            if (auto* ti = std::get_if<WasmVM::WasmInstr::TwoIdx>(&instr.imm)) {
                if (ti->a < r.mem.size()) ti->a = r.mem[ti->a];
            }
            break;
        }

        // memory.copy: (dst_memidx, src_memidx).
        case Op::Memory_copy: {
            if (auto* ti = std::get_if<WasmVM::WasmInstr::TwoIdx>(&instr.imm)) {
                if (ti->a < r.mem.size()) ti->a = r.mem[ti->a];
                if (ti->b < r.mem.size()) ti->b = r.mem[ti->b];
            }
            break;
        }

        // Block-typed instructions: BlockType holds an optional typeidx
        // (only set for value-result blocks; wvmcc never emits one today).
        case Op::Block:
        case Op::Loop:
        case Op::If:
        case Op::Try_table: {
            if (auto* bt = std::get_if<WasmVM::WasmInstr::BlockType>(&instr.imm)) {
                if (bt->type.has_value() && *bt->type < r.type.size()) {
                    bt->type = r.type[*bt->type];
                }
            }
            break;
        }

        default:
            break;
    }

    // MemArg-bearing load/store ops: memidx remap.
    if (auto* ma = std::get_if<WasmVM::WasmInstr::MemArg>(&instr.imm)) {
        if (ma->memidx < r.mem.size()) ma->memidx = r.mem[ma->memidx];
    }
}

void mergeOne(LinkContext& ctx, const WasmVM::WasmModule& in,
              const std::string& origin,
              const std::vector<DataPtrSite>& dataRelocs,
              const std::vector<DataPtrSite>& funcPtrRelocs) {
    auto& out = ctx.output;
    DedupState& ds = dedupFor(ctx);

    // Import counts before this module — used to shift already-merged
    // definitions up by however many new imports this module introduces.
    const WasmVM::index_t oldFuncImports   = ds.funcImportCount;
    const WasmVM::index_t oldGlobalImports = ds.globalImportCount;

    // ---- M2-L8: choose this TU's data rebase delta ----
    // Each TU emits its data starting at offset 8, so without rebasing they
    // collide. Pack this TU's data block immediately after everything placed
    // so far; `dataDelta` is the uniform shift applied to its data segments AND
    // to the i64.const data pointers in its code (via `dataRelocs`).
    uint64_t dataDelta = 0;
    uint64_t tuMin = UINT64_MAX, tuMax = 0;
    {
        bool hasData = false;
        auto note = [&](uint64_t lo, uint64_t hi) {
            tuMin = std::min(tuMin, lo);
            tuMax = std::max(tuMax, hi);
            hasData = true;
        };
        // Initialized data: the segment ranges.
        for (const auto& d : in.datas) {
            if (!d.mode.offset.has_value()) continue;
            uint64_t off = dataSegOffset(d);
            note(off, off + (uint64_t)d.init.size());
        }
        // BSS / zero-initialized objects emit no data segment, but the code and
        // address-globals that reference them carry their addresses as
        // i64.const. Fold those in so a BSS-only TU (e.g. malloc.c, whose only
        // datum is `static unsigned long __heap_offset`) is still rebased to a
        // non-colliding region. (+8: at least a word; aggregate objects also
        // appear in a data segment above, so this lower bound suffices.)
        for (const auto& s : dataRelocs) {
            if (s.funcIdx >= in.funcs.size()) continue;
            const auto& body = in.funcs[s.funcIdx].body;
            if (s.instrIdx >= body.size()) continue;
            const auto& instr = body[s.instrIdx];
            if (instr.opcode != WasmVM::Opcode::I64_const) continue;
            if (auto* c = std::get_if<WasmVM::WasmInstr::ConstI64>(&instr.imm))
                note((uint64_t)c->value, (uint64_t)c->value + 8);
        }
        for (const auto& g : in.globals) {
            if (auto* c = std::get_if<WasmVM::Instr::I64_const>(&g.init))
                note((uint64_t)c->value, (uint64_t)c->value + 8);
        }
        if (hasData) {
            if (ds.dataTop == 0) {
                ds.dataTop = tuMax;          // first TU keeps its layout
            } else {
                uint64_t newBase = (ds.dataTop + 7u) & ~uint64_t{7};
                dataDelta = newBase - tuMin;
                ds.dataTop = newBase + (tuMax - tuMin);
            }
        }
    }
    // True if `v` is an address into this TU's data block — used to relocate
    // address-valued global initializers (the cross-TU address-globals a TU
    // exports for &object) by the same delta as the data they point at.
    auto inTuData = [&](uint64_t v) { return dataDelta != 0 && v >= tuMin && v < tuMax; };
    // funcIdx → instr positions of i64.const data pointers to shift.
    std::unordered_map<uint32_t, std::vector<uint32_t>> relocByFunc;
    if (dataDelta != 0)
        for (const auto& s : dataRelocs) relocByFunc[s.funcIdx].push_back(s.instrIdx);

    Remap r;

    // ---- Imports: dedupe by (module, name), assigning to the right idx ---
    // We process in order so each input's import indices map to the same
    // output index any prior input would have used.
    for (const auto& imp : in.imports) {
        ImportKey key{imp.module, imp.name};

        if (std::holds_alternative<WasmVM::index_t>(imp.desc)) {
            auto it = ds.funcImports.find(key);
            if (it != ds.funcImports.end()) {
                r.func.push_back(it->second);
                continue;
            }
            // Re-intern the function type through the current output.
            WasmVM::index_t inTypeIdx = std::get<WasmVM::index_t>(imp.desc);
            if (inTypeIdx >= in.types.size()) {
                ctx.error(origin + ": import '" + imp.module + "." + imp.name +
                          "' references out-of-range type index");
                return;
            }
            WasmVM::index_t outTypeIdx = internType(out, in.types[inTypeIdx]);
            WasmVM::WasmImport newImp = imp;
            newImp.desc = outTypeIdx;
            out.imports.push_back(std::move(newImp));
            WasmVM::index_t outIdx = ds.funcImportCount++;
            ds.funcImports[key] = outIdx;
            r.func.push_back(outIdx);
        } else if (std::holds_alternative<WasmVM::MemType>(imp.desc)) {
            auto it = ds.memImports.find(key);
            if (it != ds.memImports.end()) {
                r.mem.push_back(it->second);
                continue;
            }
            out.imports.push_back(imp);
            WasmVM::index_t outIdx = ds.memImportCount++;
            ds.memImports[key] = outIdx;
            r.mem.push_back(outIdx);
        } else if (std::holds_alternative<WasmVM::GlobalType>(imp.desc)) {
            auto it = ds.globalImports.find(key);
            if (it != ds.globalImports.end()) {
                r.global.push_back(it->second);
                continue;
            }
            out.imports.push_back(imp);
            WasmVM::index_t outIdx = ds.globalImportCount++;
            ds.globalImports[key] = outIdx;
            r.global.push_back(outIdx);
        } else if (std::holds_alternative<WasmVM::TableType>(imp.desc)) {
            auto it = ds.tableImports.find(key);
            if (it != ds.tableImports.end()) {
                r.table.push_back(it->second);
                continue;
            }
            out.imports.push_back(imp);
            WasmVM::index_t outIdx = ds.tableImportCount++;
            ds.tableImports[key] = outIdx;
            r.table.push_back(outIdx);
        }
    }

    // ---- Re-index prior definitions for any imports just added ----
    // New func/global imports occupy slots before all defined funcs/globals,
    // so every reference in the already-merged output to a definition shifts
    // up by the count of imports this module introduced.
    shiftExistingDefRefs(out,
                         oldFuncImports,   ds.funcImportCount   - oldFuncImports,
                         oldGlobalImports, ds.globalImportCount - oldGlobalImports);

    // ---- Types: dedupe ----
    // Build typeRemap separately from import path so internal type uses are
    // remapped consistently. Function imports above already deduped types
    // they reference — but only for the imports themselves. We still need
    // a full input-typeIdx → output-typeIdx mapping for function bodies.
    for (size_t i = 0; i < in.types.size(); ++i) {
        r.type.push_back(internType(out, in.types[i]));
    }

    // ---- Linkable-mode invariants: no local memories / tables ----
    if (!in.mems.empty()) {
        ctx.error(origin + ": linkable input defines its own memory; merge in "
                           "linkable mode expects mem imports only");
        return;
    }
    // ---- #79: merge per-TU funcref tables into one unified table 0 ----
    // Each input may define a local funcref table (sized to its function-pointer
    // slots, or empty when it only *calls* through pointers). We concatenate the
    // slots: this input's entries land at [slotDelta, slotDelta + size), so its
    // element-segment offset and its function-pointer i64.const constants are
    // shifted by slotDelta below. crt0 later renumbers element-segment ref.func
    // entries by the sys_proc import count; the slot constants are table indices,
    // independent of the function index space, so they stay put.
    const uint64_t slotDelta = ds.slotTop;
    {
        uint64_t inSlots = 0;
        for (const auto& t : in.tables) {
            if (t.reftype != WasmVM::RefType::funcref) {
                ctx.error(origin + ": linkable input defines a non-funcref table "
                                   "(unsupported)");
                return;
            }
            inSlots += t.limits.min;
        }
        if (!in.tables.empty()) {
            if (out.tables.empty()) {
                WasmVM::TableType u;
                u.limits.min = (WasmVM::offset_t)(ds.slotTop + inSlots);
                u.limits.max = std::nullopt;
                u.limits.is64 = false;
                u.reftype = WasmVM::RefType::funcref;
                out.tables.push_back(u);
            } else {
                out.tables[0].limits.min =
                    (WasmVM::offset_t)(ds.slotTop + inSlots);
                if (out.tables[0].limits.max.has_value())
                    out.tables[0].limits.max =
                        (WasmVM::offset_t)(ds.slotTop + inSlots);
            }
        }
        ds.slotTop += inSlots;
    }

    // Map this input's funcPtr relocation sites by input-local function index.
    std::unordered_map<uint32_t, std::vector<uint32_t>> funcPtrByFunc;
    if (slotDelta != 0)
        for (const auto& s : funcPtrRelocs)
            funcPtrByFunc[s.funcIdx].push_back(s.instrIdx);

    // ---- Globals: append defined globals (after import dedup is done) ----
    // M2-D linkable modules have zero defined globals. Defensive support
    // for future libc TUs that might add some.
    WasmVM::index_t prevDefinedGlobalCount = (WasmVM::index_t)out.globals.size();
    for (const auto& g : in.globals) {
        WasmVM::index_t outIdx =
            ds.globalImportCount + prevDefinedGlobalCount + (WasmVM::index_t)(r.global.size() - ds.globalImportCount);
        (void)outIdx;
        WasmVM::WasmGlobal copy = g;
        remapConstInstr(copy.init, r);
        // M2-L8: an i64.const global initializer that addresses this TU's data
        // (an exported address-global, e.g. &__wvmcc_stdout) must move with the
        // rebased data.
        if (auto* c = std::get_if<WasmVM::Instr::I64_const>(&copy.init)) {
            if (inTuData((uint64_t)c->value)) c->value += (WasmVM::i64_t)dataDelta;
        }
        out.globals.push_back(std::move(copy));
        // Defined-global output index = number of global imports + position
        // in out.globals (already pushed → size - 1).
        r.global.push_back(ds.globalImportCount + (WasmVM::index_t)(out.globals.size() - 1));
    }

    // ---- Functions: assign defined-func output indices BEFORE walking
    // bodies, so within-TU calls resolve correctly. ----
    WasmVM::index_t prevDefinedFuncCount = (WasmVM::index_t)out.funcs.size();
    // Reserve slots in r.func for defined functions.
    for (size_t i = 0; i < in.funcs.size(); ++i) {
        WasmVM::index_t outIdx = ds.funcImportCount + prevDefinedFuncCount + (WasmVM::index_t)i;
        r.func.push_back(outIdx);
    }

    // Now emit each function with its body's indices remapped.
    for (size_t i = 0; i < in.funcs.size(); ++i) {
        WasmVM::WasmFunc f = in.funcs[i];
        // Remap typeidx.
        if (f.typeidx >= r.type.size()) {
            ctx.error(origin + ": function references out-of-range type index");
            return;
        }
        f.typeidx = r.type[f.typeidx];
        // Remap each instruction.
        for (auto& instr : f.body) {
            remapInstr(instr, r);
        }
        // M2-L8: shift this function's data-pointer i64.const constants by the
        // TU's data rebase delta.
        if (dataDelta != 0) {
            auto it = relocByFunc.find((uint32_t)i);
            if (it != relocByFunc.end()) {
                for (uint32_t pos : it->second) {
                    if (pos >= f.body.size()) continue;
                    auto& instr = f.body[pos];
                    if (instr.opcode == WasmVM::Opcode::I64_const) {
                        if (auto* c = std::get_if<WasmVM::WasmInstr::ConstI64>(&instr.imm))
                            c->value += (WasmVM::i64_t)dataDelta;
                    }
                }
            }
        }
        // #79: rebase function-pointer i64.const slots by the TU's table delta,
        // preserving the tag in the high nibble.
        if (slotDelta != 0) {
            auto it = funcPtrByFunc.find((uint32_t)i);
            if (it != funcPtrByFunc.end()) {
                for (uint32_t pos : it->second) {
                    if (pos >= f.body.size()) continue;
                    auto& instr = f.body[pos];
                    if (instr.opcode == WasmVM::Opcode::I64_const) {
                        if (auto* c = std::get_if<WasmVM::WasmInstr::ConstI64>(&instr.imm)) {
                            int64_t slot = c->value & kFuncPtrSlotMask;
                            c->value = kFuncPtrTag | (slot + (int64_t)slotDelta);
                        }
                    }
                }
            }
        }
        out.funcs.push_back(std::move(f));
    }

    // ---- Data segments: append; rebase offsets by the TU's data delta
    // (M2-L8) so multi-TU data doesn't collide. Remap memidx in the mode. ----
    for (const auto& d : in.datas) {
        WasmVM::WasmData copy = d;
        if (copy.mode.memidx.has_value()) {
            auto idx = *copy.mode.memidx;
            if (idx < r.mem.size()) copy.mode.memidx = r.mem[idx];
        }
        if (copy.mode.offset.has_value()) {
            remapConstInstr(*copy.mode.offset, r);
            if (dataDelta != 0) {
                uint64_t off = dataSegOffset(copy);
                copy.mode.offset = WasmVM::Instr::I64_const{(WasmVM::i64_t)(off + dataDelta)};
            }
        }
        out.datas.push_back(std::move(copy));
    }

    // ---- Element segments: append; remap tableidx and funcref entries, and
    // shift the active offset into this input's slice of the unified table
    // (#79). The ref.func entries are remapped via r.func. ----
    for (const auto& e : in.elems) {
        WasmVM::WasmElem copy = e;
        if (copy.mode.tableidx.has_value()) {
            auto idx = *copy.mode.tableidx;
            if (idx < r.table.size()) copy.mode.tableidx = r.table[idx];
        }
        if (copy.mode.offset.has_value()) {
            remapConstInstr(*copy.mode.offset, r);
            // #79: place this segment at [slotDelta + originalOffset).
            if (slotDelta != 0) {
                if (auto* c = std::get_if<WasmVM::Instr::I32_const>(&*copy.mode.offset))
                    c->value += (WasmVM::i32_t)slotDelta;
                else if (auto* c64 = std::get_if<WasmVM::Instr::I64_const>(&*copy.mode.offset))
                    c64->value += (WasmVM::i64_t)slotDelta;
            }
        }
        for (auto& entry : copy.elemlist) {
            remapConstInstr(entry, r);
        }
        out.elems.push_back(std::move(copy));
    }

    // ---- Exports: remap and append; dedupe by name (error on collision).
    for (const auto& ex : in.exports) {
        for (const auto& existing : out.exports) {
            if (existing.name == ex.name) {
                ctx.error(origin + ": duplicate export name '" + ex.name +
                          "' (already exported by a prior input)");
                return;
            }
        }
        WasmVM::WasmExport copy = ex;
        switch (copy.desc) {
            case WasmVM::WasmExport::DescType::func:
                if (copy.index < r.func.size()) copy.index = r.func[copy.index];
                break;
            case WasmVM::WasmExport::DescType::global:
                if (copy.index < r.global.size()) copy.index = r.global[copy.index];
                break;
            case WasmVM::WasmExport::DescType::mem:
                if (copy.index < r.mem.size()) copy.index = r.mem[copy.index];
                break;
            case WasmVM::WasmExport::DescType::table:
                if (copy.index < r.table.size()) copy.index = r.table[copy.index];
                break;
        }
        out.exports.push_back(std::move(copy));
    }

    // ---- Start: error if any input declares one in linkable mode (M2-L6
    // produces the final start). ----
    if (in.start.has_value()) {
        ctx.error(origin + ": linkable input has a start section; in linkable "
                           "mode start is synthesized by crt0 (M2-L6)");
        return;
    }
}

} // namespace wvmcc::link::merge
