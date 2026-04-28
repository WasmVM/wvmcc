# Phase 4: Advanced Features Implementation Subtasks

## Overview
This document outlines the detailed implementation subtasks for Phase 4 of the C17 to WasmVM lowering pipeline, which adds support for function pointers, static locals, and forward goto.

## Subtasks

### Step 4.1 — Function table and `Call_indirect`
- [ ] Add `WasmTableType` (funcref) to `mod_.tables`
- [ ] First pass: assign table slot to every non-`static` function whose address is taken
- [ ] Implement `emitExpr` for:
  - `Unary{op="&", inner=FuncIdent}` → `i64.const{tableSlot}`
- [ ] Implement `emitExpr` for:
  - `Call` with non-identifier callee → emit callee (table index), emit args, `Call_indirect{tableIdx, typeIdx}`

### Step 4.2 — `static` locals
- [ ] Implement `static` local declaration handling:
  - `static` local declaration → `GlobalDataAllocator` path (like file-scope aggregate)
  - Assign a `WasmGlobal` or a `WasmData` slot in `mem[0]`
  - Emit address as i64 constant
- [ ] Implement single initialization via a guard global flag

### Step 4.3 — Forward-only `goto`
- [ ] Implement structural lifting:
  - Wrap the goto target and all code between goto and label in a `Block`
  - Replace `goto L` with `Br{depth}`
- [ ] Implement restriction:
  - Backward goto (loop) → emit `Unreachable` + diagnostic (not supported)

### Step 4.4 — Designated initializers
- [ ] Implement `emit` for:
  - Struct/array initializer bytes in field-offset order, respecting designators
- [ ] Requires `LayoutEngine` (already done in Phase 2)

### Step 4.5 — `_Bool` and `_Complex` basics
- [ ] Implement `_Bool` handling:
  - i32 local; on store, normalize to 0/1 via `i32.const 0` + compare (`i32.ne`)
- [ ] Implement `_Complex` handling:
  - Pair of f32/f64 locals (real + imaginary)
  - Basic arithmetic only

### Step 4.6 — Verification
- [ ] Compile callback-passing C file; verify `call_indirect` with `readwasm --func --table output.wasm`
- [ ] Compile `static`-local counter function; verify single initialization via `readwasm --global output.wasm`
- [ ] Run via `wasmvm output.wasm`; regression: all unit tests pass