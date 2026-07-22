`v1.0.0`
# v1.0.0 — First release: a freestanding C17 → WasmVM toolchain

wvmcc compiles C17 straight to a WasmVM `WasmModule` — preprocessing, parsing, semantic
analysis, code generation, and linking in one binary, with no external assembler or linker.

Highlights:

- **Complete pipeline**: C17 translation phases 1–7, hand-written recursive-descent parser,
  semantic analysis, and direct-to-`WasmModule` codegen (wasm64, LP64; dual-memory model with
  tagged pointers for cross-memory dereference).
- **Integrated linker**: multi-TU and `.o`/`.wasm` object merging, symbol resolution, lazy
  `.a` archive members, relocations, dead-code elimination, `crt0` synthesis, link maps.
- **Freestanding libc**: built by wvmcc itself into a sysroot `libc.a` — stdio, stdlib,
  string, ctype, and math with the documented errno contract.
- **Conformance**: tracked against ISO/IEC 9899:2017 in `docs/standard/`; the 526-test suite
  runs 520/520 `status-supported` rows green (the 2 deferred failures are the intended
  conformance signal for not-yet-diagnosed `_Thread_local`/VLA constraints).
- **Docs**: architecture overview with pipeline diagram (`docs/lowering-pipeline.md`), data
  model and ABI (`docs/spec.md`), codegen and linker design docs.

Quick start:

```sh
wvmcc hello.c -o hello.wasm && wasmvm hello.wasm
```

**WasmVM compatibility**: built and smoke-tested against WasmVM v1.4 (the `.deb` depends on
the `wasmvm` package; install it from the WasmVM releases page).
