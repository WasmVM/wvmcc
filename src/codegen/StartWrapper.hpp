#pragma once

#include <WasmVM.hpp>

namespace wvmcc::codegen::startwrapper {

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
void emitStartWrapper(WasmVM::WasmModule& module,
                      const SysProcImports& sysProc,
                      WasmVM::index_t mainFuncIdx,
                      bool mainHasArgv);

} // namespace wvmcc::codegen::startwrapper
