# C17 → WasmVM Lowering Pipeline

## Context

wvmcc has a complete C17 parser and semantic analysis but produces empty `.wasm` binaries — `main.cpp:208` constructs an empty `WasmVM::WasmModule` with no code emission. This plan adds a `src/codegen/` layer that traverses the C17 AST and emits a populated `WasmModule` using the WasmVM struct API directly.

**Threads are deferred** — no `_Thread_local`, pthreads, or atomic operations in this plan.

**Freestanding implementation** — `main` is not required. No function is unconditionally exported; all non-`static` functions are exported by default (freestanding library convention).

---

## Type Mapping

| C type | Wasm `ValueType` |
|---|---|
| `_Bool`, `char`, `short`, `int`, `unsigned` variants, `enum` | `i32` |
| `long`, `unsigned long`, `long long`, `unsigned long long`, any pointer | `i64` |
| `float` | `f32` |
| `double`, `long double` | `f64` |
| `struct`, `union`, array | memory-resident; represented by `i64` address |
| `void` | no return value |

Target is **Wasm64** — all pointers are `i64` linear memory addresses. `sizeof(void*) == 8`.

`long` is 64-bit on Wasm64 (LP64 model). `int` remains 32-bit.

Narrow integer load/store: `char` → `I32_load8_s`/`I32_store8`, `short` → `I32_load16_s`/`I32_store16`, `int` → `I32_load`/`I32_store`, pointer/`long`/`long long` → `I64_load`/`I64_store`.

---

## Memory Model

Memory indices use a 4-bit namespace (0–15), allowing up to 16 distinct memories for future use. Currently assigned:

```
memidx  Purpose
──────  ─────────────────────────────────────────────────────
  0     Heap + Static data (Wasm64 i64 addresses)
          [0..7]          reserved (null pointer sentinel)
          [8..static_end) static data: aggregates, string literals (BSS/rodata)
          [static_end..)  future heap (malloc)
  1     Shadow stack (Wasm64 i64 addresses, grows downward)
          [top..bottom]   call frames (address-taken locals, aggregate locals)
 2–15   Reserved for future use (e.g. separate rodata, thread-local storage, etc.)
```

- **Scalar locals** (not address-taken) → Wasm locals (`WasmFunc::locals`)
- **Address-taken locals + all aggregates** → shadow stack in `mem[1]` (`MemoryLocal{frame_offset}`)
- **File-scope scalars** → `WasmGlobal` entries
- **File-scope aggregates + string literals** → active `WasmData` segments in `mem[0]` at static addresses
- **`__stack_pointer`** → mutable `i64` `WasmGlobal`, initialized to `mem[1]` top (e.g. `0x10000`)

Stack addresses (into `mem[1]`) and heap/static addresses (into `mem[0]`) are distinct i64 address spaces — no collision possible.

Load/store instructions carry the memory index explicitly:
- Global/heap/static data: `memidx=0`
- Shadow-stack frame locals: `memidx=1`

`makeLoad(type, memidx)` and `makeStore(type, memidx)` accept the memory index as a parameter so future memory slots can be introduced without changing call sites.

Function prologue/epilogue emit `global.get/set $__stack_pointer` to allocate/free the frame in `mem[1]`.

---

## New Source Files (`src/codegen/`)

| File | Responsibility |
|---|---|
| `TypeMap.hpp/.cpp` | `toWasmType(TypeNodePtr)`, `byteSize()`, `byteAlignment()`, `isMemoryResident()`, load/store instruction selection (with `memidx` parameter) |
| `LayoutEngine.hpp/.cpp` | C17 struct field offsets and padding; cached by `StructOrUnionSpecifier*` |
| `SymbolTable.hpp` | Scope-stacked map: name → `VarInfo` variant (`ScalarLocal`, `MemoryLocal`, `GlobalScalar`, `GlobalMem`, `FuncSymbol`) |
| `TypeIndexCache.hpp` | Deduplicate `FuncType` → `index_t` in `WasmModule::types` |
| `GlobalDataAllocator.hpp` | Assign byte offsets for static data; owns `string_literal_pool` |
| `AddressTakenAnalyzer.hpp/.cpp` | Pre-pass over a `FunctionDefPtr` AST; returns `std::unordered_set<string>` of address-taken names |
| `FunctionCodegen.hpp/.cpp` | Per-function code generator; produces `WasmFunc`; owns instruction buffer + `ControlFlowStack` |
| `ModuleCodegen.hpp/.cpp` | Top-level driver; iterates TU externals; assembles final `WasmModule` |

---

## Expression Lowering

All expression emission is stack-machine style: `emitExpr(ExprPtr, bool need_lvalue)` pushes one value (scalar) or an `i32` address (aggregate/lvalue).

| Expression | Wasm output |
|---|---|
| `Integer`, `Char` | `I32_const` (fits in 32 bits) or `I64_const` (suffixed `LL`/`ULL`) |
| `Float` | `F32_const` / `F64_const` |
| `String` | `I64_const{static_addr}` (from `string_literal_pool`) |
| `Ident` (scalar local, int/enum) | `Local_get{idx}` → `i32` local |
| `Ident` (scalar local, long/ptr) | `Local_get{idx}` → `i64` local |
| `Ident` (memory local, need value) | `local.get $fp` + `i64.const offset` + `i64.add` + load from `mem[1]` |
| `Ident` (global scalar) | `Global_get{idx}` |
| `&x` | `emitExpr(x, need_lvalue=true)` — x is guaranteed `MemoryLocal` by pre-pass |
| `*p` | emit p (`i64` address), then load (or return address if `need_lvalue`) |
| Binary arithmetic | emit lhs, rhs, then typed op (`I32_add`, `I64_add`, `F64_mul`, etc.) |
| Pointer arithmetic | extend integer operand to `i64`, multiply by `sizeof(*pointee)`, then `I64_add` |
| `=` | `emitExpr(lhs, lvalue=true)`, emit rhs, store |
| `&&` | `if (result i32) { rhs; eqz; eqz } else { i32.const 0 } end` |
| `\|\|` | `if (result i32) { i32.const 1 } else { rhs; eqz; eqz } end` |
| Ternary | `if (result T) { then } else { else } end` |
| `.member` / `->member` | base address (`i64`) + `I64_const{field_offset}` + `I64_add`, then load from appropriate memory |
| `a[i]` | same as `*(a + i * sizeof(element))` with `i64` address arithmetic |
| `Cast` | conversion instruction: `I64_extend_i32_s`, `F32_convert_i64_s`, `I32_wrap_i64`, etc. |
| `sizeof`, `_Alignof` | `I64_const{compile_time_value}` (returns `size_t` = `i64` on Wasm64) |
| `Call` (direct) | emit args, `Call{func_idx}` |
| `GenericSelection` | fold at codegen time; lower the selected association |

---

## Statement Lowering

| Statement | Wasm pattern |
|---|---|
| `if` / `if-else` | `If` / `Else` / `End` |
| `while` | `Block $break` → `Loop $cont` → cond `Br_if $break` → body → `Br $cont` → `End End` |
| `do-while` | `Block $break` → `Loop $cont` → body → cond `Br_if $cont` → `End End` |
| `for` | init; same as `while` |
| `return` | emit value (if any), `Return` |
| `break` / `continue` | `Br{depth}` — depth computed from `ControlFlowStack` |
| `switch` (dense) | `br_table` dispatch; nested `Block`s per case, outermost `Block` is break target |
| `switch` (sparse) | chained `if`/`else` comparisons (when `max-min > 4 * num_cases`) |
| `goto` (forward only) | Phase 4: `Block`+`Br` restructuring |
| `_Static_assert` | no code emitted (evaluated at semantic phase) |

---

## Calling Convention

- **Scalar params/return** → direct Wasm param/result types
- **Struct params** → caller copies to its shadow stack, passes `i32` address
- **Struct return** → hidden first `i32` param (caller-allocated buffer address)
- **`extern` declarations** → `WasmImport{module="env", name=func_name, desc=type_idx}`
- **Non-`static` functions** → exported via `WasmExport` (freestanding library convention; no special treatment of `main`)
- **`static` functions** → not exported
- **Variadic functions** → deferred (emit `Unreachable` + diagnostic for now)

---

## Integration Point

**`src/exec/main.cpp` lines 207–208** — replace empty module construction:

```cpp
// Before:
WasmVM::WasmModule module;

// After:
#include "../codegen/ModuleCodegen.hpp"
wvmcc::codegen::ModuleCodegen codegen(sem);
auto module = codegen.generate(tu);
if (auto err = WasmVM::module_validate(module)) {
    /* emit diagnostic, return 1 */
}
```

**`CMakeLists.txt`** — add one line to the existing source glob:

```cmake
${SRC_ROOT}/codegen/*.cpp
```

WasmVM headers are already on the include path via `WASMVM_INCLUDE_DIR` (pointing to `/Users/luishsu/Desktop/WasmVM/src/include/`).

---

## Phased Delivery

### Phase 1 — Scalar Foundation
**Milestone**: compile `int add(int a, int b) { return a+b; }` to valid `.wasm`

#### Step 1.1 — Build infrastructure (no codegen yet)
- Create `src/codegen/` directory
- `TypeMap.hpp/.cpp`: `toWasmType()`, `byteSize()`, `byteAlignment()`, `isMemoryResident()`, `makeLoad(type, memidx)`, `makeStore(type, memidx)` — Wasm64 rules (pointer/long → i64, int → i32)
- `SymbolTable.hpp`: `VarInfo` variant + scope-stack (`pushScope`/`popScope`/`define`/`lookup`)
- `TypeIndexCache.hpp`: `intern(FuncType)` with deduplication
- `GlobalDataAllocator.hpp`: `allocate(size, align)`, `internString()`, `currentTop()`
- Add `${SRC_ROOT}/codegen/*.cpp` to `CMakeLists.txt` source glob

#### Step 1.2 — ModuleCodegen skeleton
- `ModuleCodegen.hpp/.cpp`: constructor + `generate()` shell
- Emit two `MemType` entries: `mem[0]` (heap/static, 1 page min), `mem[1]` (shadow stack, 1 page min); indices 2–15 reserved
- Emit `__stack_pointer` mutable i64 global (init = top of `mem[1]`, e.g. `0x10000`)
- First pass: iterate TU externals; register every function definition and `extern` function declaration in `symtab_` and `mod_.imports`/`mod_.funcs`; export all non-`static` functions
- First pass: register file-scope scalar variables as `WasmGlobal` (i64, mutable, init 0)
- Second pass: placeholder — leave function bodies empty for now

#### Step 1.3 — FunctionCodegen skeleton + integration
- `FunctionCodegen.hpp/.cpp`: constructor, `generate()` shell, `allocLocal()`, `emit()`
- Wire `generate()` into `ModuleCodegen` second pass
- Patch `src/exec/main.cpp:208`: replace empty `WasmModule` with `ModuleCodegen::generate(tu)`; surface `module_validate` errors as diagnostics
- Build must compile (empty function bodies → just `End`)

#### Step 1.4 — Expression lowering (scalars)
- `emitExpr`: `Integer`, `Char` → `I32_const`; string/pointer-sized integer → `I64_const`
- `emitExpr`: `Ident` (ScalarLocal) → `Local_get`; (GlobalScalar) → `Global_get`
- `emitExpr`: arithmetic `Binary` (`+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`) — i32 and i64 variants selected by operand type
- `emitExpr`: comparison `Binary` (`==`, `!=`, `<`, `>`, `<=`, `>=`) → i32 result
- `emitExpr`: unary `-`, `~`, `!`, `+`
- `emitExpr`: `Cast` — emit conversion instructions (`I64_extend_i32_s`, `I32_wrap_i64`, `F32_convert_i64_s`, etc.)

#### Step 1.5 — Statement lowering (basic control flow)
- `emitStmt`: `Return` (with and without value)
- `emitStmt`: `Expr` statement (emit expr, `Drop` result)
- `emitStmt`: `Compound` (push/pop scope, iterate block items)
- `emitStmt`: `If` / `If-Else` → `If` / `Else` / `End`
- `emitStmt`: `While` → `Block` + `Loop` + `Br_if` + `Br` + `End End`
- `emitStmt`: `For` → init block-item + same pattern as `while`
- `emitBlockItem`: `Declaration` → allocate `ScalarLocal` (i32 or i64 by type), emit initializer if present

#### Step 1.6 — Direct function calls
- `emitExpr`: `Call` with identifier callee → emit args, `Call{funcIdx}`
- Verify call site type matches registered `FuncType`

#### Step 1.7 — Verification
- Compile `int add(int a, int b) { return a+b; }` → `wasm-validate output.wasm` passes
- Run existing parser/semantic unit tests — all pass
- Smoke-test `extern` import: compile `extern int puts(int s); int greet() { return puts(0); }` → validate

---

### Phase 2 — Memory and Aggregates
**Milestone**: structs, arrays, pointers, string literals work

#### Step 2.1 — AddressTakenAnalyzer
- `AddressTakenAnalyzer.hpp/.cpp`: pre-pass over a `FunctionDefPtr`; walk all `UnaryExpr{op="&"}` subtrees; return `std::unordered_set<std::string>` of address-taken names
- Run at the start of `FunctionCodegen::generate()`; promote those names to `MemoryLocal` instead of `ScalarLocal`

#### Step 2.2 — Shadow stack prologue/epilogue
- In `FunctionCodegen::generate()`: if any `MemoryLocal` exists, emit prologue:
  `global.get $__stack_pointer` → `local.tee $fp` → `i64.const frameSize` → `i64.sub` → `global.set $__stack_pointer`
- Emit epilogue before every `Return`: `local.get $fp` → `global.set $__stack_pointer`
- `MemoryLocal` address = `local.get $fp` + `i64.const offset` + `i64.add` (accessing `mem[1]`)

#### Step 2.3 — LayoutEngine
- `LayoutEngine.hpp/.cpp`: compute C17 struct field byte offsets with alignment padding; cache result by `StructOrUnionSpecifier*`
- `byteSize()` / `byteAlignment()` in `TypeMap` delegate to `LayoutEngine` for struct/union

#### Step 2.4 — Pointer and address expressions
- `emitExpr`: `Ident` (MemoryLocal, need_value) → load from `mem[1]` at frame offset
- `emitExpr`: `Ident` (MemoryLocal, need_lvalue) → push i64 address in `mem[1]`
- `emitExpr`: `Unary{op="&"}` → `emitExpr(inner, need_lvalue=true)`
- `emitExpr`: `Unary{op="*"}` → emit pointer, load from `mem[0]` (or return address if need_lvalue)
- Pointer arithmetic: extend integer operand to i64, multiply by `byteSize(pointee)`, `i64.add`

#### Step 2.5 — Member and index access
- `emitExpr`: `Member` (`.` and `->`) → base address + `i64.const{field_offset}` + `i64.add`; load if need_value, pass address if need_lvalue
- `emitExpr`: `Index` (`a[i]`) → equivalent to `*(a + i * sizeof(*a))`; i64 address arithmetic; load/store via `mem[0]`

#### Step 2.6 — String literals and static aggregates
- `emitExpr`: `String` → `GlobalDataAllocator::internString()`; emit `i64.const{addr}` (addr in `mem[0]`)
- After second pass: emit active `WasmData` segments (targeting `mem[0]`) for each interned string
- File-scope aggregate initializers → allocate in `GlobalDataAllocator`; emit `WasmData` segment with initializer bytes

#### Step 2.7 — Struct param/return ABI
- Struct param: caller allocates buffer on its shadow stack (`mem[1]`), copies struct, passes i64 address
- Struct return: hidden first i64 param (caller-allocated buffer in `mem[1]`)
- Update `FuncType` construction in `ModuleCodegen` first pass to apply ABI transformation

#### Step 2.8 — CompoundLiteral
- `emitExpr`: `CompoundLiteral` → allocate on shadow stack (`mem[1]`); emit initializer bytes; push i64 address

#### Step 2.9 — Verification
- Compile struct-using C file; inspect with `wasm-objdump -d`
- Compile pointer-arithmetic C file; verify loads/stores target correct memory indices
- Regression: all unit tests pass

---

### Phase 3 — Control Flow Completeness
**Milestone**: `switch`, `break`/`continue`, short-circuit `&&`/`||`, ternary, `do-while`, post-inc/dec

#### Step 3.1 — ControlFlowStack
- Add `ControlFlowStack` to `FunctionCodegen`: each entry records kind (`Block`/`Loop`/`Switch`) and label depth
- Replace ad-hoc `cfStack_` with this; compute break/continue depths correctly through nested constructs

#### Step 3.2 — `break` and `continue`
- `emitStmt`: `Break` → `Br{breakDepth()}` using `ControlFlowStack`
- `emitStmt`: `Continue` → `Br{continueDepth()}` using `ControlFlowStack`

#### Step 3.3 — `do-while`
- `emitStmt`: `DoWhile` → `Block $break` + `Loop $cont` + body + cond + `Br_if $cont` + `End End`

#### Step 3.4 — Short-circuit `&&` / `||`
- `emitExpr`: `Binary{op="&&"}` → `if (result i32) { rhs; eqz; eqz } else { i32.const 0 } end`; use `BlockInstr` with i32 result type
- `emitExpr`: `Binary{op="||"}` → `if (result i32) { i32.const 1 } else { rhs; eqz; eqz } end`

#### Step 3.5 — Ternary
- `emitExpr`: `Ternary` → `if (result T) { then } else { else } end` with correct block result type

#### Step 3.6 — Post-increment / post-decrement
- `emitExpr`: `PostfixUnary{Inc/Dec}` → load old value into scratch local; increment/decrement and store back; push scratch local

#### Step 3.7 — `switch` (dense)
- `SwitchCaseCollector`: walk `switch` body, collect `case` values and their block depths
- Dense case (`max−min ≤ 4 × num_cases`): emit `br_table` dispatch; nested `Block`s per case; outermost `Block` is break target
- Fall-through between cases handled by not emitting `Br` at case end

#### Step 3.8 — `switch` (sparse)
- Sparse case (`max−min > 4 × num_cases`): emit chained `if`/`else` comparisons

#### Step 3.9 — Verification
- Compile switch/loop-heavy C file; verify control flow in WAT dump
- Regression: all unit tests pass

---

### Phase 4 — Advanced Features
**Milestone**: function pointers, `static` locals, forward `goto`

#### Step 4.1 — Function table and `Call_indirect`
- Add `WasmTableType` (funcref) to `mod_.tables`
- First pass: assign table slot to every non-`static` function whose address is taken
- `emitExpr`: `Unary{op="&", inner=FuncIdent}` → `i64.const{tableSlot}`
- `emitExpr`: `Call` with non-identifier callee → emit callee (table index), emit args, `Call_indirect{tableIdx, typeIdx}`

#### Step 4.2 — `static` locals
- `static` local declaration → `GlobalDataAllocator` path (like file-scope aggregate)
- Assign a `WasmGlobal` or a `WasmData` slot in `mem[0]`; emit address as i64 constant
- Initializer emitted once (first call) via a guard global flag

#### Step 4.3 — Forward-only `goto`
- Structural lifting: wrap the goto target and all code between goto and label in a `Block`; replace `goto L` with `Br{depth}`
- Restriction: backward goto (loop) → emit `Unreachable` + diagnostic (not supported)

#### Step 4.4 — Designated initializers
- Emit struct/array initializer bytes in field-offset order, respecting designators
- Requires `LayoutEngine` (already done in Phase 2)

#### Step 4.5 — `_Bool` and `_Complex` basics
- `_Bool`: i32 local; on store, normalize to 0/1 via `i32.const 0` + compare (`i32.ne`)
- `_Complex`: pair of f32/f64 locals (real + imaginary); basic arithmetic only

#### Step 4.6 — Verification
- Compile callback-passing C file; verify `call_indirect` in WAT dump
- Compile `static`-local counter function; verify single initialization
- Regression: all unit tests pass

---

### Phase 5 — Robustness
**Milestone**: diagnostics, validation, BSS zero-init, integration tests

#### Step 5.1 — Unimplemented path coverage
- Every unimplemented `emitExpr`/`emitStmt` branch: emit `Unreachable` and push a `Diagnostic{Error, "codegen not implemented: <feature>"}` — no silent wrong code

#### Step 5.2 — BSS zero-init
- After second pass: if any static BSS (zero-init) data exists, emit a Wasm start function that calls `memory.fill` on `mem[0]` over the BSS range

#### Step 5.3 — `module_validate` integration
- Already wired in Step 1.3; ensure all validation errors are surfaced as `Diagnostic{Error}` with source location where possible

#### Step 5.4 — Integration tests
- Write a suite of small `.c` files covering each phase milestone
- Build and run each via WasmVM; verify return values match expected output
- Regression: all existing parser/semantic unit tests pass

---

## Critical Files

| File | Role |
|---|---|
| `src/exec/main.cpp` | Integration point (lines 207–208) |
| `CMakeLists.txt` | Add codegen glob |
| `src/parser/AST.hpp` | AST types used throughout |
| `src/parser/Semantic.hpp` | `typeOfExpr()`, `buildTypeFromDeclaration()` |
| `src/parser/ASTVisitor.hpp` | Base class for traversal |
| `WasmVM/src/include/structures/WasmInstr.hpp` | All instruction types |
| `WasmVM/src/include/structures/WasmModule.hpp` | Module struct |
| `WasmVM/src/include/structures/WasmFunc.hpp` | Function struct |
| `WasmVM/src/include/Types.hpp` | `ValueType`, `FuncType`, `index_t` |

---

## Verification

1. **Phase 1**: `wasm-validate output.wasm` passes; run via WasmVM
2. **Phase 2**: Compile a struct-using C file; inspect with `wasm-objdump -d`; verify `mem[0]`/`mem[1]` load/store indices
3. **Phase 3**: Compile a switch/loop-heavy C file; verify control flow in WAT dump
4. **Phase 4**: Compile callback and `static`-local C files; verify `call_indirect` and single-init in WAT dump
5. **Phase 5**: Full integration test suite run via WasmVM
6. **Regression**: All existing parser/semantic unit tests pass after each phase
