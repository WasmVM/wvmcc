# WVMCC Spec (Freestanding, WasmModule IR)

## Goals
- Freestanding C17 compiler (no libc, no WASI). No reliance on host syscalls; only pure computation and explicit imports chosen by us.
- Output format: WasmVM `WasmModule` as the final IR/module. Internal compilation can use a custom IR before lowering.
- Initial focus: correctness and clean architecture over optimizations.

## Language Scope (M0 → M2)
- M0: Core C17 expressions/statements, scalar types, pointers, arrays, functions, structs/unions/enums, qualifiers, usual arithmetic conversions, control flow, function prototypes.
- M1: Declarations + initializers (scalars/aggregates), compound literals, designators, flexible array members, bitfields (documented layout).
- M2: Variadics (ABI-limited), `_Static_assert`, `_Alignof`, `_Noreturn`, `_Generic` later.
- Deferred initially: VLAs, `_Complex`, atomics (`_Atomic`, `<stdatomic.h>`), `<threads.h>`.

## Freestanding Constraints
- No standard library. Programs may not call `printf`, `malloc`, etc.
- Entry point: configurable (e.g., `_start` or `main`) but no runtime provided by us in M0.
- Environment: linear memory only; any I/O must be via explicit imports the user defines.

## Data Layout & ABI (wasm32)
- Endianness: little-endian.
- Pointer size: 32-bit; `size_t`/`ptrdiff_t` are 32-bit.
- Alignment: natural (1/2/4/8). Struct layout follows wasm32/Clang-like natural alignment; document padding.
- `char` signedness: implementation-defined; default to signed (`-funsigned-char` available later).
- `long double`: initially aliased to `double`.
- Variadics: supported with wasm32-compatible ABI; validate alignment rules via tests.

## Architecture
- Frontend:
  - Lexer: C17 tokens, tracks `typedef` names.
  - Parser: hand-written recursive descent with declarator grammar; recovery at `;`/`}`/`,`.
  - AST: typed nodes, source spans; casts inserted during semantic analysis.
  - Semantics: scope stacks (file/block/function/tag), type system (qualifiers, arrays, functions, pointers), conversions, constant folding, diagnostics.
- IR:
  - Custom IR: SSA-like blocks with simple three-address ops for expressions/control flow.
  - Passes: const-fold, copy-prop, dead-code elim (basic), canonicalization.
- Backend:
  - Final output is `WasmModule` using `wasmvm_include` headers you provided.
  - Lower custom IR → WasmVM op set (`i32/i64/f32/f64`), linear memory for aggregates.
  - Calling convention: wasm32-compatible; spill to linear memory as needed.
  - No host imports in M0; expose only minimal module sections.

## Developer Workflows
- Build: `mkdir build && cd build && cmake .. && make -j4`
- Run: `wvmcc source.c -o out.wasm` (CLI emits WasmModule → wasm or native WasmVM format depending on support).
- Tests:
  - Unit: C++ tests for lexer/parser/semantics/IR.
  - E2E: compile small freestanding programs (no I/O), e.g., arithmetic, branches, function calls; validate results by reading memory/return values via a test harness.

## Project Conventions
- C++20 only; minimal dependencies.
- Tree:
  - `src/{lexer,parser,ast,semantics}/`
  - `src/ir/`
  - `src/exec/`
  - `include/` (public headers)
  - `wasmvm_include/` (WasmVM headers; not vendored by us)
  - `tests/{unit,e2e}/`

## Milestones
- M0: Parse/type-check core C; IR gen for expressions/statements; lower to minimal `WasmModule`; run pure computation programs in WasmVM.
- M1: Initializers/aggregates/struct layout tests; function calls across files; basic diagnostics.
- M2: Variadics, `_Alignof/_Static_assert`, bitfields; improved IR passes.

## Open Topics
- Exact mapping of struct/union layout to WasmVM memory: confirm rules against Clang wasm32 and freeze.
- Module format details: decide whether to emit `.wasm` or a WasmVM-native serialization; wrap with a loader utility.
- CLI flags: `-target wasm32`, `-funsigned-char`, `-fno-long-double`, output selection.
