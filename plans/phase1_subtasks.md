# Phase 1 Implementation Plan: Scalar Foundation - Focused on Expression and Statement Lowering

## Overview
This plan focuses specifically on implementing the expression and statement lowering components for Phase 1 of the lowering plan, which aims to compile `int add(int a, int b) { return a+b; }` to a valid `.wasm` file.

## Current State Analysis
Based on my analysis, the following components already exist but are mostly placeholders:
- All codegen files (TypeMap, SymbolTable, TypeIndexCache, GlobalDataAllocator, ModuleCodegen, FunctionCodegen) exist
- The main implementation is mostly placeholders that emit Unreachable instructions
- The project structure and build system are already in place

## Focused Implementation Tasks

### Step 1.4 — Expression Lowering (Scalars) - Focused Implementation
- [ ] Implement `emitExpr` for:
  - `Integer`, `Char` → `I32_const`; string/pointer-sized integer → `I64_const`
  - `Ident` (ScalarLocal) → `Local_get`
  - `Ident` (GlobalScalar) → `Global_get`
  - Arithmetic `Binary` (`+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`) with i32 and i64 variants selected by operand type
  - Comparison `Binary` (`==`, `!=`, `<`, `>`, `<=`, `>=`) → i32 result
  - Unary `-`, `~`, `!`, `+`
  - `Cast` → emit conversion instructions (`I64_extend_i32_s`, `I32_wrap_i64`, `F32_convert_i64_s`, etc.)
- [ ] Implement proper type inference for expressions
- [ ] Handle expression evaluation order correctly

### Step 1.5 — Statement Lowering (Basic Control Flow) - Focused Implementation
- [ ] Implement `emitStmt` for:
  - `Return` (with and without value)
  - `Expr` statement (emit expr, `Drop` result)
  - `Compound` (push/pop scope, iterate block items)
  - `If` / `If-Else` → `If` / `Else` / `End`
  - `While` → `Block` + `Loop` + `Br_if` + `Br` + `End End`
  - `For` → init block-item + same pattern as `while`
- [ ] Implement `emitBlockItem` for:
  - `Declaration` → allocate `ScalarLocal` (i32 or i64 by type), emit initializer if present
- [ ] Handle proper control flow stack management

### Step 1.6 — Direct Function Calls - Focused Implementation
- [ ] Implement `emitExpr` for:
  - `Call` with identifier callee → emit args, `Call{funcIdx}`
- [ ] Verify call site type matches registered `FuncType`

## Detailed Implementation Requirements

### Expression Lowering Requirements
1. **Integer Literals**: 
   - Handle `Integer` and `Char` expressions
   - Emit appropriate constants (`I32_const` for 32-bit, `I64_const` for 64-bit)
   - Handle proper type promotion (e.g., char to int)

2. **Identifier Expressions**:
   - Handle `Ident` expressions for local variables (`ScalarLocal`)
   - Emit `Local_get` instructions
   - Handle global scalar variables with `Global_get`

3. **Arithmetic Operations**:
   - Implement all binary arithmetic operations (`+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`)
   - Select appropriate Wasm instructions based on operand types (i32 vs i64)
   - Handle proper type promotion and conversion

4. **Comparison Operations**:
   - Implement all comparison operations (`==`, `!=`, `<`, `>`, `<=`, `>=`)
   - All return i32 results (as per Wasm specification)
   - Handle proper type checking and conversion

5. **Unary Operations**:
   - Implement unary `-`, `~`, `!`, `+` operations
   - Handle proper type conversion and sign extension

6. **Cast Operations**:
   - Implement proper casting between types
   - Emit appropriate conversion instructions (`I64_extend_i32_s`, `I32_wrap_i64`, etc.)

### Statement Lowering Requirements
1. **Return Statements**:
   - Handle return statements with and without values
   - Emit appropriate `Return` instructions
   - Handle proper type conversion for return values

2. **Expression Statements**:
   - Emit expressions and drop the result if needed
   - Handle proper stack management

3. **Compound Statements**:
   - Implement scope management (push/pop)
   - Iterate through block items in order

4. **Control Flow Statements**:
   - `If` / `If-Else`: Emit proper conditional structures (`If` / `Else` / `End`)
   - `While`: Emit loop structure (`Block` + `Loop` + `Br_if` + `Br` + `End End`)
   - `For`: Implement loop with initialization block

5. **Declaration Handling**:
   - Handle local variable declarations
   - Allocate `ScalarLocal` variables with proper types (i32 or i64)
   - Emit initializers if present

## Implementation Strategy
1. **Start with basic expression types** (integer literals, identifiers)
2. **Progress to arithmetic and comparison operations**
3. **Implement unary operations and casting**
4. **Add control flow statements** (if, while, for)
5. **Handle return statements and compound blocks**
6. **Test with simple function compilation** (e.g., `int add(int a, int b) { return a+b; }`)
7. **Verify module validation passes**

## Testing Approach
- Create unit tests for each expression type to ensure proper Wasm instruction generation
- Test with simple functions like `int add(int a, int b) { return a+b; }`
- Verify that generated Wasm modules pass validation
- Run existing parser/semantic unit tests to ensure no regressions

## Integration Points
- `src/exec/main.cpp` lines 207–208: Replace empty module construction with `ModuleCodegen::generate(tu)`
- `CMakeLists.txt`: Add `${SRC_ROOT}/codegen/*.cpp` to source glob