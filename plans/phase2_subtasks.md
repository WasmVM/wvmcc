# Phase 2 Implementation Plan: Memory and Aggregates

## Overview
This plan outlines the implementation of Phase 2 for the wvmcc project, which focuses on adding memory and aggregate type support (structs, arrays, pointers, etc.) to the code generation pipeline.

## Key Components

### 1. AddressTakenAnalyzer
- **File**: `src/codegen/AddressTakenAnalyzer.hpp` and `src/codegen/AddressTakenAnalyzer.cpp`
- **Responsibility**: Pre-pass over a `FunctionDefPtr` AST to identify variables that have their address taken (`&` operator)
- **Implementation**:
  - Walk all `UnaryExpr{op="&"}` subtrees
  - Return `std::unordered_set<std::string>` of address-taken names
  - Run at the start of `FunctionCodegen::generate()`

### 2. LayoutEngine
- **File**: `src/codegen/LayoutEngine.hpp` and `src/codegen/LayoutEngine.cpp`
- **Responsibility**: Compute C17 struct field byte offsets with alignment padding and cache results
- **Implementation**:
  - Compute C17 struct field byte offsets with alignment padding
  - Cache result by `StructOrUnionSpecifier*`
  - Update `TypeMap` to delegate `byteSize()` / `byteAlignment()` to `LayoutEngine` for struct/union

### 3. Memory and Aggregate Support in FunctionCodegen
- **Enhancements to**: `src/codegen/FunctionCodegen.hpp` and `src/codegen/FunctionCodegen.cpp`
- **Implementation**:
  - Promote address-taken names to `MemoryLocal` instead of `ScalarLocal`
  - In `FunctionCodegen::generate()`: if any `MemoryLocal` exists, emit prologue:
    - `global.get $__stack_pointer` → `local.tee $fp` → `i64.const frameSize` → `i64.sub` → `global.set $__stack_pointer`
  - Emit epilogue before every `Return`:
    - `local.get $fp` → `global.set $__stack_pointer`
  - `MemoryLocal` address = `local.get $fp` + `i64.const offset` + `i64.add` (accessing `mem[1]`)

### 4. Expression Emission for Memory Operations
- **Enhancements to**: `src/codegen/FunctionCodegen.cpp`
- **Implementation**:
  - `emitExpr` for: `Ident` (MemoryLocal, need_value) → load from `mem[1]` at frame offset
  - `emitExpr` for: `Ident` (MemoryLocal, need_lvalue) → push i64 address in `mem[1]`
  - `emitExpr` for: `Unary{op="&"}` → `emitExpr(inner, need_lvalue=true)`
  - `emitExpr` for: `Unary{op="*"}` → emit pointer, load from `mem[0]` (or return address if need_lvalue)
  - Pointer arithmetic: extend integer operand to i64, multiply by `byteSize(pointee)`, `i64.add`
  - `emitExpr` for: `Member` (`.` and `->`) → base address + `i64.const{field_offset}` + `i64.add`; load if need_value, pass address if need_lvalue
  - `emitExpr` for: `Index` (`a[i]`) → equivalent to `*(a + i * sizeof(*a))`; i64 address arithmetic; load/store via `mem[0]`
  - `emitExpr` for: `String` → `GlobalDataAllocator::internString()`; emit `i64.const{addr}` (addr in `mem[0]`)

### 5. Data Segment Emission
- **Enhancements to**: `src/codegen/ModuleCodegen.cpp`
- **Implementation**:
  - After second pass: emit active `WasmData` segments (targeting `mem[0]`) for each interned string
  - File-scope aggregate initializers → allocate in `GlobalDataAllocator`; emit `WasmData` segment with initializer bytes

### 6. Struct ABI Handling
- **Enhancements to**: `src/codegen/ModuleCodegen.cpp`
- **Implementation**:
  - Struct parameter handling: caller allocates buffer on its shadow stack (`mem[1]`), copies struct, passes i64 address
  - Struct return handling: hidden first i64 param (caller-allocated buffer in `mem[1]`)
  - Update `FuncType` construction in `ModuleCodegen` first pass to apply ABI transformation

### 7. Compound Literal Support
- **Enhancements to**: `src/codegen/FunctionCodegen.cpp`
- **Implementation**:
  - `emitExpr` for: `CompoundLiteral` → allocate on shadow stack (`mem[1]`); emit initializer bytes; push i64 address

## Implementation Steps

### Step 1: Create AddressTakenAnalyzer
- [ ] Create `src/codegen/AddressTakenAnalyzer.hpp`
- [ ] Create `src/codegen/AddressTakenAnalyzer.cpp`
- [ ] Implement AST traversal to find address-taken variables

### Step 2: Create LayoutEngine
- [ ] Create `src/codegen/LayoutEngine.hpp`
- [ ] Create `src/codegen/LayoutEngine.cpp`
- [ ] Implement struct field layout calculation with proper alignment

### Step 3: Update TypeMap to use LayoutEngine
- [ ] Modify `src/codegen/TypeMap.hpp` and `src/codegen/TypeMap.cpp`
- [ ] Update `byteSize()` / `byteAlignment()` to delegate to `LayoutEngine` for struct/union

### Step 4: Update FunctionCodegen for Memory Support
- [ ] Modify `src/codegen/FunctionCodegen.hpp` and `src/codegen/FunctionCodegen.cpp`
- [ ] Implement prologue/epilogue generation for functions with memory locals
- [ ] Add logic to promote address-taken variables to `MemoryLocal`

### Step 5: Implement Memory Expression Emission
- [ ] Add new expression emission logic in `src/codegen/FunctionCodegen.cpp`
- [ ] Handle `&` operator, pointer dereference, member access, array indexing

### Step 6: Update ModuleCodegen for Data Segments
- [ ] Modify `src/codegen/ModuleCodegen.cpp` to emit data segments for strings and aggregates

### Step 7: Implement Struct ABI Handling
- [ ] Modify `src/codegen/ModuleCodegen.cpp` to handle struct parameters and returns
- [ ] Update function type construction for ABI compliance

### Step 8: Add Compound Literal Support
- [ ] Add `CompoundLiteral` expression handling in `src/codegen/FunctionCodegen.cpp`

### Step 9: Verification and Testing
- [ ] Compile struct-using C file; inspect with `readwasm --func --data output.wasm` to confirm field offsets and load/store memory indices
- [ ] Compile pointer-arithmetic C file; verify `mem[0]`/`mem[1]` indices in `readwasm` output
- [ ] Run via `wasmvm output.wasm`; regression: all unit tests pass

## Integration Points
- `src/exec/main.cpp` lines 207–208: Replace empty module construction with `ModuleCodegen::generate(tu)`
- `CMakeLists.txt`: Add `${SRC_ROOT}/codegen/*.cpp` to source glob (already done in Phase 1)