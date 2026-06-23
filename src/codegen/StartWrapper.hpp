#pragma once

#include <WasmVM.hpp>

#include <optional>

namespace wvmcc::codegen::startwrapper {

// #98: shadow-stack (mem[1]) sizing. The shadow stack holds C call frames AND
// the crt0-built argv[] block (emitStartWrapper writes argv into mem[1] at
// SP-relative offsets). mem[1] is sized to the largest single frame plus a
// fixed reserve for call-depth nesting and argv, and `__stack_pointer` is
// initialized to the top of mem[1] (the stack grows downward). Both the
// freestanding compiler (ModuleCodegen) and the linker (Crt0Synth) use these.
constexpr uint64_t kWasmPageSize = 65536;
// Reserve on top of the largest single frame. One page preserves the legacy
// 64 KiB stack budget for call depth + argv, while the largest frame gets its
// own room above it — so this strictly dominates the old fixed 1-page stack.
constexpr uint64_t kShadowStackReserve = kWasmPageSize;
// Page-rounded mem[1] byte size for a module whose largest frame is maxFrame.
// The stack pointer is initialized to this value.
inline uint64_t shadowStackSize(uint64_t maxFrame) {
    uint64_t pages = (maxFrame + kShadowStackReserve + kWasmPageSize - 1) / kWasmPageSize;
    if (pages < 1) pages = 1;
    return pages * kWasmPageSize;
}

// #98: high-nibble tag marking an i64 pointer value as referring to mem[1] (the
// shadow stack). Mirrors FunctionCodegen::kMemidxShift — argv pointers handed to
// `main` must carry this tag so user-code deref dispatches to mem[1], and the
// tag-aware sysenv host (get_mem) writes argv strings into mem[1].
constexpr int64_t kMem1PtrTag = (int64_t)1 << 60;

// Indices of the four sys_proc imports (in `module.imports` order = function
// index space, since imports occupy the low end of the index space).
struct SysProcImports {
    WasmVM::index_t argc;     // ()       -> i32
    WasmVM::index_t argvLen;  // (i32)    -> i32
    WasmVM::index_t argv;     // (i32, i64, i64) -> i32
    WasmVM::index_t exit;     // (i32)    -> ()
};

// Append the four sys_proc imports to `module`. Increments `nextFuncIndex`
// past them. Returns the four allocated function indices.
//
// Caller must ensure no user functions have been registered yet — the
// sys_proc imports must occupy a contiguous low range of the function
// index space.
SysProcImports injectSysProcImports(WasmVM::WasmModule& module,
                                    int& nextFuncIndex);

// Build and append a `start`-section wrapper to `module`. The wrapper:
//   * if mainHasArgv: builds argc / argv[] on the shadow stack, then calls
//     `main(argc, argv)` and forwards the i32 result to sys_proc.exit.
//   * else: calls `main()` and forwards the i32 result to sys_proc.exit.
//
// Also exports `main` as a function. Mutates module.funcs, module.start,
// module.exports.
//
// `mainFuncIdx` is the final index of `main` in the function index space
// (linker callers may renumber it during merging — pass the post-merge
// index).
//
// `libcExitIdx`, when set, is the function index of libc's `_Noreturn void
// exit(int)`. The wrapper then terminates by calling it with main's result, so
// returning from main runs the same atexit-handler path as an explicit exit()
// (#79) — including stdio's self-registered flush. When unset, the wrapper
// falls back to the optional `atExitFlushIdx` cleanup plus sys_proc.exit.
//
// `atExitFlushIdx`, when set (and `libcExitIdx` is not), is the function index
// of a `() -> ()` cleanup (libc's `__stdio_exit`) called after `main` returns
// and before sys_proc.exit, so buffered stdio is flushed on normal termination.
void emitStartWrapper(WasmVM::WasmModule& module,
                      const SysProcImports& sysProc,
                      WasmVM::index_t mainFuncIdx,
                      bool mainHasArgv,
                      std::optional<WasmVM::index_t> atExitFlushIdx = std::nullopt,
                      std::optional<WasmVM::index_t> libcExitIdx = std::nullopt);

} // namespace wvmcc::codegen::startwrapper
