#include <cstdint>
#include "Crt0Synth.hpp"
#include "../codegen/StartWrapper.hpp"

#include <algorithm>
#include <optional>
#include <variant>

namespace wvmcc::link::crt0 {

namespace {

// Shift every defined / imported function-index reference in the module
// up by `shift`. Used to make room for sys_proc imports prepended at the
// start of the function index space.
void shiftFunctionIndices(WasmVM::WasmModule& m, WasmVM::index_t shift) {
    namespace Op = WasmVM::Opcode;
    auto shiftConstInstr = [&](WasmVM::ConstInstr& c) {
        std::visit([&](auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, WasmVM::Instr::Ref_func>) {
                v.index += shift;
            }
        }, c);
    };

    for (auto& f : m.funcs) {
        for (auto& instr : f.body) {
            switch (instr.opcode) {
                case Op::Call:
                case Op::Return_call:
                case Op::Ref_func: {
                    if (auto* oi = std::get_if<WasmVM::WasmInstr::OneIdx>(&instr.imm)) {
                        oi->index += shift;
                    }
                    break;
                }
                default: break;
            }
        }
    }
    for (auto& ex : m.exports) {
        if (ex.desc == WasmVM::WasmExport::DescType::func) ex.index += shift;
    }
    for (auto& e : m.elems) {
        for (auto& entry : e.elemlist) shiftConstInstr(entry);
    }
    for (auto& g : m.globals) {
        shiftConstInstr(g.init);
    }
    if (m.start.has_value()) {
        m.start = *m.start + shift;
    }
}

bool isEnvImport(const WasmVM::WasmImport& imp, const std::string& name) {
    return imp.module == "env" && imp.name == name;
}

// Pull main's funcidx and whether it takes (argc, argv) out of the merged
// output's exports + types. Returns std::nullopt if main isn't exported.
struct MainInfo {
    WasmVM::index_t funcIdx;
    bool hasArgv;
};
std::optional<MainInfo> findMain(const WasmVM::WasmModule& m) {
    for (const auto& ex : m.exports) {
        if (ex.name == "main" && ex.desc == WasmVM::WasmExport::DescType::func) {
            // Resolve typeidx via the function's body slot. Function index
            // space layout: imports first, then defined funcs.
            WasmVM::index_t funcImports = 0;
            for (const auto& imp : m.imports) {
                if (std::holds_alternative<WasmVM::index_t>(imp.desc)) ++funcImports;
            }
            if (ex.index < funcImports) {
                // Imported main? Shouldn't happen for linked binaries.
                return std::nullopt;
            }
            WasmVM::index_t defIdx = ex.index - funcImports;
            if (defIdx >= m.funcs.size()) return std::nullopt;
            WasmVM::index_t typeidx = m.funcs[defIdx].typeidx;
            if (typeidx >= m.types.size()) return std::nullopt;
            const auto& ft = m.types[typeidx];
            // hasArgv when first two params are (i32, i64) (per M1 ABI).
            bool hasArgv = ft.params.size() == 2 &&
                           ft.params[0] == WasmVM::ValueType::i32 &&
                           ft.params[1] == WasmVM::ValueType::i64;
            return MainInfo{ex.index, hasArgv};
        }
    }
    return std::nullopt;
}

// Look up a defined (non-imported) function export by name, returning its
// function index. Used to discover libc's optional `__stdio_exit` cleanup so
// crt0 can call it on normal termination — present only when stdio was linked.
std::optional<WasmVM::index_t> findExportedFunc(const WasmVM::WasmModule& m,
                                                const std::string& name) {
    for (const auto& ex : m.exports) {
        if (ex.name == name && ex.desc == WasmVM::WasmExport::DescType::func) {
            return ex.index;
        }
    }
    return std::nullopt;
}

} // namespace

void synthesize(LinkContext& ctx) {
    auto& m = ctx.output;

    if (ctx.opts.no_stdlib) {
        // -nostdlib: skip crt0. The user is responsible for providing
        // runtime state and a start function. Just leave the module as-is.
        return;
    }

    auto mainInfo = findMain(m);
    if (!mainInfo) {
        ctx.error("link: no exported 'main' function found "
                  "(use -nostdlib if you don't need crt0)");
        return;
    }

    // #79: prefer terminating through libc `exit`, so returning from main runs
    // atexit handlers (including stdio's self-registered flush) exactly like an
    // explicit exit() call. The lazy-pull phase seeds `exit` for executables, so
    // it is present whenever libc is linked. Fall back to the direct
    // `__stdio_exit` flush when there is no libc exit (e.g. -nostdlib images).
    // Both are captured pre-shift (like main) so the +kSysProcCount adjustment
    // applies after the sys_proc imports renumber the function index space.
    auto exitInfo  = findExportedFunc(m, "exit");
    auto flushInfo = exitInfo ? std::nullopt : findExportedFunc(m, "__stdio_exit");

    // ----- 1. Drop env.__* imports of mem/global kind. They'll be
    //          replaced by local definitions at the same index positions.
    //          Memory imports are env.__linear_memory (mem[0]),
    //          env.__stack_memory (mem[1]) and, for wvmcc_memidx(N) placements,
    //          env.__memory_N (mem[N]). Count them so we recreate exactly as
    //          many local memories, preserving every memory index.
    WasmVM::index_t droppedMems = 0;
    {
        std::vector<WasmVM::WasmImport> kept;
        kept.reserve(m.imports.size());
        for (const auto& imp : m.imports) {
            const bool isMem    = std::holds_alternative<WasmVM::MemType>(imp.desc);
            const bool isGlobal = std::holds_alternative<WasmVM::GlobalType>(imp.desc);
            if (isMem && imp.module == "env" &&
                (imp.name == "__linear_memory" || imp.name == "__stack_memory" ||
                 imp.name.rfind("__memory_", 0) == 0)) {
                ++droppedMems;
                continue; // drop
            }
            if (isGlobal && imp.module == "env" &&
                (imp.name == "__stack_pointer" || imp.name == "__heap_base")) {
                continue; // drop
            }
            kept.push_back(imp);
        }
        m.imports = std::move(kept);
    }

    // ----- 2. Add local mems and globals. mem[0] = heap, mem[1] = stack, and
    //          mem[2..] = wvmcc_memidx(N) placement memories (one per dropped
    //          env memory import). All use the M1 size (1 page each).
    //          __heap_base is computed from the merged mem[0] data top (data
    //          segments still carry their TU-local offsets — M2-L8 rebases
    //          multi-TU; single-TU is already correct).
    auto memTy = []() {
        WasmVM::MemType t;
        t.min = 1;
        t.is64 = true;
        return t;
    }();
    if (droppedMems < 2) droppedMems = 2; // heap + stack always exist
    // Linkable inputs carry no local memories, so the merged module has none
    // here; prepend `droppedMems` local memories at indices 0..droppedMems-1.
    for (WasmVM::index_t i = 0; i < droppedMems; ++i) {
        m.mems.insert(m.mems.begin() + i, memTy);
    }

    // Stack pointer: mut i64 initialized to one page (0x10000).
    {
        WasmVM::WasmGlobal sp;
        sp.type = WasmVM::GlobalType{WasmVM::GlobalType::variable,
                                     WasmVM::ValueType::i64};
        sp.init = WasmVM::Instr::I64_const{0x10000};
        m.globals.insert(m.globals.begin(), sp);
    }
    // Heap base: const i64 = round_up_8(max data-segment end).
    {
        uint64_t top = 0;
        for (const auto& d : m.datas) {
            // Only mem[0] holds the heap; placement memories (mem[2..]) have
            // their own address spaces and must not inflate __heap_base.
            if (d.mode.memidx.value_or(0) != 0) continue;
            uint64_t base = 0;
            if (d.mode.offset.has_value()) {
                std::visit([&](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, WasmVM::Instr::I64_const>) {
                        base = (uint64_t)v.value;
                    } else if constexpr (std::is_same_v<T, WasmVM::Instr::I32_const>) {
                        base = (uint64_t)v.value;
                    }
                }, *d.mode.offset);
            }
            uint64_t end = base + d.init.size();
            top = std::max(top, end);
        }
        top = (top + 7u) & ~uint64_t{7};
        WasmVM::WasmGlobal hb;
        hb.type = WasmVM::GlobalType{WasmVM::GlobalType::constant,
                                     WasmVM::ValueType::i64};
        hb.init = WasmVM::Instr::I64_const{(WasmVM::i64_t)top};
        m.globals.insert(m.globals.begin() + 1, hb);
    }

    // The codegen-side "main" export served as our hint to find main —
    // remove it now so the start wrapper's own "main" export doesn't
    // collide. (The start wrapper re-adds the export at the correct
    // post-shift index.)
    m.exports.erase(
        std::remove_if(m.exports.begin(), m.exports.end(),
            [](const WasmVM::WasmExport& ex) {
                return ex.name == "main" &&
                       ex.desc == WasmVM::WasmExport::DescType::func;
            }),
        m.exports.end());

    // ----- 3. Prepend sys_proc function imports. This shifts every
    //          existing function index by +4 (since 4 new function imports
    //          take positions 0..3 of the function index space).
    constexpr WasmVM::index_t kSysProcCount = 4;
    shiftFunctionIndices(m, kSysProcCount);

    // Build the 4 sys_proc imports by hand (M2-F's helper writes to a
    // module from scratch with `nextFuncIndex` starting at 0; here we
    // want to insert at the FRONT).
    auto findOrInternType = [&](const WasmVM::FuncType& want) -> WasmVM::index_t {
        for (WasmVM::index_t i = 0; i < (WasmVM::index_t)m.types.size(); ++i) {
            if (m.types[i].params == want.params &&
                m.types[i].results == want.results) {
                return i;
            }
        }
        m.types.push_back(want);
        return (WasmVM::index_t)(m.types.size() - 1);
    };
    auto makeSysProcImport = [&](const std::string& name,
                                 std::vector<WasmVM::ValueType> params,
                                 std::vector<WasmVM::ValueType> results) {
        WasmVM::FuncType ft;
        ft.params = std::move(params);
        ft.results = std::move(results);
        WasmVM::index_t typeidx = findOrInternType(ft);
        WasmVM::WasmImport imp;
        imp.module = "sys_proc";
        imp.name = name;
        imp.desc = typeidx;
        return imp;
    };
    using VT = WasmVM::ValueType;
    std::vector<WasmVM::WasmImport> sysProcImports;
    sysProcImports.push_back(makeSysProcImport("argc",     {},                          {VT::i32}));
    sysProcImports.push_back(makeSysProcImport("argv_len", {VT::i32},                   {VT::i32}));
    sysProcImports.push_back(makeSysProcImport("argv",     {VT::i32, VT::i64, VT::i64}, {VT::i32}));
    sysProcImports.push_back(makeSysProcImport("exit",     {VT::i32},                   {}));
    m.imports.insert(m.imports.begin(),
                     sysProcImports.begin(), sysProcImports.end());

    // ----- 4. Emit the start wrapper. The M2-F helper is parameterized by
    //          the SysProcImports indices + main's funcidx + whether main
    //          takes argv.
    using namespace wvmcc::codegen::startwrapper;
    SysProcImports sp{0, 1, 2, 3};
    // main's funcidx was shifted earlier by +4; mainInfo.funcIdx holds the
    // pre-shift value, so add the shift.
    WasmVM::index_t mainShifted = mainInfo->funcIdx + kSysProcCount;
    std::optional<WasmVM::index_t> flushShifted;
    if (flushInfo) flushShifted = *flushInfo + kSysProcCount;
    std::optional<WasmVM::index_t> exitShifted;
    if (exitInfo) exitShifted = *exitInfo + kSysProcCount;
    emitStartWrapper(m, sp, mainShifted, mainInfo->hasArgv, flushShifted,
                     exitShifted);
}

} // namespace wvmcc::link::crt0
