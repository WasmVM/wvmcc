#include "StartWrapper.hpp"

#include <utility>
#include <vector>

namespace wvmcc::codegen::startwrapper {

namespace {

// Local copy of module-type interning so this helper has no dependency on
// ModuleCodegen state. Operates directly on module.types.
WasmVM::index_t internFuncType(WasmVM::WasmModule& module, const WasmVM::FuncType& ft) {
    for (WasmVM::index_t i = 0; i < (WasmVM::index_t)module.types.size(); ++i) {
        if (module.types[i] == ft) return i;
    }
    module.types.push_back(ft);
    return (WasmVM::index_t)(module.types.size() - 1);
}

WasmVM::index_t registerImport(WasmVM::WasmModule& module,
                               int& nextFuncIndex,
                               const std::string& name,
                               std::vector<WasmVM::ValueType> params,
                               std::vector<WasmVM::ValueType> results) {
    WasmVM::FuncType ft;
    ft.params = std::move(params);
    ft.results = std::move(results);
    WasmVM::index_t typeidx = internFuncType(module, ft);
    WasmVM::WasmImport imp;
    imp.module = "sys_proc";
    imp.name = name;
    imp.desc = typeidx;
    module.imports.push_back(imp);
    return (WasmVM::index_t)(nextFuncIndex++);
}

} // namespace

SysProcImports injectSysProcImports(WasmVM::WasmModule& module,
                                    int& nextFuncIndex) {
    using VT = WasmVM::ValueType;
    SysProcImports r;
    r.argc    = registerImport(module, nextFuncIndex, "argc",     {},                          {VT::i32});
    r.argvLen = registerImport(module, nextFuncIndex, "argv_len", {VT::i32},                   {VT::i32});
    r.argv    = registerImport(module, nextFuncIndex, "argv",     {VT::i32, VT::i64, VT::i64}, {VT::i32});
    r.exit    = registerImport(module, nextFuncIndex, "exit",     {VT::i32},                   {});
    return r;
}

void emitStartWrapper(WasmVM::WasmModule& module,
                      const SysProcImports& sysProc,
                      WasmVM::index_t mainFuncIdx,
                      bool mainHasArgv,
                      std::optional<WasmVM::index_t> atExitFlushIdx,
                      std::optional<WasmVM::index_t> libcExitIdx) {
    // FuncType: () -> ()
    WasmVM::FuncType ft;
    WasmVM::index_t wrapperType = internFuncType(module, ft);

    // Terminal sequence after `main` returns (its i32 result is on the stack).
    // Preferred path (#79): hand the result to libc `exit`, which runs atexit
    // handlers (including stdio's self-registered flush) then never returns — so
    // normal return from main and explicit exit() share one termination path.
    // Fallback (no libc exit linked, e.g. -nostdlib): optional stdio flush then
    // sys_proc.exit directly.
    auto emitTerminate = [&](std::vector<WasmVM::WasmInstr>& b) {
        if (libcExitIdx) {
            b.push_back(WasmVM::Instr::Call{*libcExitIdx});
        } else {
            if (atExitFlushIdx) b.push_back(WasmVM::Instr::Call{*atExitFlushIdx});
            b.push_back(WasmVM::Instr::Call{sysProc.exit});
        }
        b.push_back(WasmVM::Instr::Unreachable{});
    };

    // Stack pointer global is module.globals[0] (M1 / freestanding setup).
    constexpr WasmVM::index_t kSpGlobal = 0;

    WasmVM::WasmFunc wrapper;
    wrapper.typeidx = wrapperType;
    auto& body = wrapper.body;

    if (mainHasArgv) {
        // Locals: [argc:i32, i:i32, len:i32, argv_base:i64, sp_save:i64]
        wrapper.locals = {
            WasmVM::ValueType::i32, // 0: argc
            WasmVM::ValueType::i32, // 1: i
            WasmVM::ValueType::i32, // 2: len
            WasmVM::ValueType::i64, // 3: argv_base
            WasmVM::ValueType::i64, // 4: sp_save
        };

        // argc = sys_proc.argc()
        body.push_back(WasmVM::Instr::Call{sysProc.argc});
        body.push_back(WasmVM::Instr::Local_set{0});
        // sp_save = sp
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::Local_set{4});
        // argv_base = sp - argc * 8;  sp = argv_base
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::Local_get{0});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::I64_const{8});
        body.push_back(WasmVM::Instr::I64_mul{});
        body.push_back(WasmVM::Instr::I64_sub{});
        body.push_back(WasmVM::Instr::Local_tee{3});
        body.push_back(WasmVM::Instr::Global_set{kSpGlobal});
        // i = 0
        body.push_back(WasmVM::Instr::I32_const{0});
        body.push_back(WasmVM::Instr::Local_set{1});
        // Block / Loop
        body.push_back(WasmVM::Instr::Block{std::nullopt});
        body.push_back(WasmVM::Instr::Loop{std::nullopt});
        // if (i >= argc) br outer (label index 1 from inside Loop)
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::Local_get{0});
        body.push_back(WasmVM::Instr::I32_ge_s{});
        body.push_back(WasmVM::Instr::Br_if{1});
        // len = sys_proc.argv_len(i)
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::Call{sysProc.argvLen});
        body.push_back(WasmVM::Instr::Local_set{2});
        // sp -= (len + 8) & ~7
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::Local_get{2});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::I64_const{8});
        body.push_back(WasmVM::Instr::I64_add{});
        body.push_back(WasmVM::Instr::I64_const{-8});
        body.push_back(WasmVM::Instr::I64_and{});
        body.push_back(WasmVM::Instr::I64_sub{});
        body.push_back(WasmVM::Instr::Global_set{kSpGlobal});
        // argv_base[i] = (mem1-tagged) sp  (store i64 pointer into mem[1]).
        // The shadow stack is mem[1]; the stored value is tagged so user code
        // dereferences argv[i] through mem[1]'s tag-dispatch (#78/#98).
        body.push_back(WasmVM::Instr::Local_get{3});
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::I64_const{8});
        body.push_back(WasmVM::Instr::I64_mul{});
        body.push_back(WasmVM::Instr::I64_add{});
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::I64_const{kMem1PtrTag});
        body.push_back(WasmVM::Instr::I64_or{});
        // 8-byte aligned: argv_base is computed from an 8-aligned stack pointer
        // and indexed by i*8 (align hint log2(8) = 3). memidx 1 = shadow stack.
        body.push_back(WasmVM::Instr::I64_store{1, 0, 3});
        // sys_proc.argv(i, mem1-tagged sp, (i64)(len + 1)). The tag routes the
        // host's string copy into mem[1] (sysenv get_mem dispatches on the tag).
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::I64_const{kMem1PtrTag});
        body.push_back(WasmVM::Instr::I64_or{});
        body.push_back(WasmVM::Instr::Local_get{2});
        body.push_back(WasmVM::Instr::I32_const{1});
        body.push_back(WasmVM::Instr::I32_add{});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::Call{sysProc.argv});
        body.push_back(WasmVM::Instr::Drop{});
        // ++i
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::I32_const{1});
        body.push_back(WasmVM::Instr::I32_add{});
        body.push_back(WasmVM::Instr::Local_set{1});
        // br loop (label 0)
        body.push_back(WasmVM::Instr::Br{0});
        body.push_back(WasmVM::Instr::End{}); // end loop
        body.push_back(WasmVM::Instr::End{}); // end block
        // call main(argc, argv_base) -> i32; flush stdio (if linked); then
        // exit. The flush is () -> (), so it leaves main's i32 result on the
        // operand stack for sys_proc.exit to consume.
        body.push_back(WasmVM::Instr::Local_get{0});
        body.push_back(WasmVM::Instr::Local_get{3});
        // argv itself is a mem[1] pointer: tag it so main's argv[i] indexing
        // dispatches loads to the shadow stack.
        body.push_back(WasmVM::Instr::I64_const{kMem1PtrTag});
        body.push_back(WasmVM::Instr::I64_or{});
        body.push_back(WasmVM::Instr::Call{mainFuncIdx});
        emitTerminate(body);
    } else {
        // main(void) wrapper: call main, then terminate (libc exit / flush +
        // sys_proc.exit). main's i32 result stays on the stack for the
        // terminator to consume.
        body.push_back(WasmVM::Instr::Call{mainFuncIdx});
        emitTerminate(body);
    }
    body.push_back(WasmVM::Instr::End{});

    // The function index space is [function imports..., defined funcs...].
    // Count only *function* imports — non-func imports (unresolved globals /
    // memories that survive into the final module) must not be counted, or
    // the wrapper's index lands past the end of the function space.
    WasmVM::index_t funcImportCount = 0;
    for (const auto& imp : module.imports) {
        if (std::holds_alternative<WasmVM::index_t>(imp.desc)) ++funcImportCount;
    }
    WasmVM::index_t wrapperIdx =
        (WasmVM::index_t)(funcImportCount + module.funcs.size());
    module.funcs.push_back(std::move(wrapper));
    module.start = wrapperIdx;

    WasmVM::WasmExport ex;
    ex.name = "main";
    ex.desc = WasmVM::WasmExport::DescType::func;
    ex.index = mainFuncIdx;
    module.exports.push_back(ex);
}

} // namespace wvmcc::codegen::startwrapper
