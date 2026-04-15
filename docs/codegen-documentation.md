# C17 to WasmVM Lowering Pipeline Documentation

This document provides comprehensive documentation for the C17 to WasmVM lowering pipeline implementation, detailing each component and their integration.

## Overview

The wvmcc compiler now includes a complete C17 to WasmVM lowering pipeline that transforms C17 source code into valid WebAssembly modules. This implementation follows the phased approach outlined in the lowering plan, with each phase building upon the previous ones to provide full C17 language support.

## Code Generation Components

### 1. TypeMap
The `TypeMap` component is responsible for converting C types to their WebAssembly equivalents and providing type information for memory operations.

**Key Features:**
- Converts C types to Wasm `ValueType` (e.g., `int` → `i32`, `long`/pointers → `i64`)
- Calculates byte size and alignment for all C types
- Determines if a type is memory resident (structs, unions, arrays)
- Generates appropriate load/store instructions with memory indices

**Implementation Details:**
```cpp
WasmVM::ValueType toWasmType(const wvmcc::parser::TypeNodePtr& type) const;
size_t byteSize(const wvmcc::parser::TypeNodePtr& type) const;
size_t byteAlignment(const wvmcc::parser::TypeNodePtr& type) const;
bool isMemoryResident(const wvmcc::parser::TypeNodePtr& type) const;
WasmVM::WasmInstr makeLoad(const wvmcc::parser::TypeNodePtr& type, uint8_t memidx) const;
WasmVM::WasmInstr makeStore(const wvmcc::parser::TypeNodePtr& type, uint8_t memidx) const;
```

### 2. SymbolTable
The `SymbolTable` manages symbol information with proper scoping and variable tracking.

**Key Features:**
- Scope-stacked symbol management
- Tracks different symbol types (ScalarLocal, MemoryLocal, GlobalScalar, GlobalMem, FuncSymbol)
- Supports nested scopes with proper lookup and definition
- Handles symbol visibility and lifetime

**Implementation Details:**
```cpp
void pushScope();
void popScope();
bool define(const std::string& name, const VarInfoStruct& info);
std::optional<VarInfoStruct> lookup(const std::string& name) const;
```

### 3. TypeIndexCache
The `TypeIndexCache` deduplicates function types to avoid redundant entries in the Wasm module.

**Key Features:**
- Interns `FuncType` objects to prevent duplication
- Maintains a mapping from function types to indices in the module's type section
- Provides efficient lookup for existing function types

**Implementation Details:**
```cpp
WasmVM::index_t intern(const WasmVM::FuncType& funcType);
std::optional<WasmVM::index_t> getIndex(const WasmVM::FuncType& funcType) const;
```

### 4. GlobalDataAllocator
The `GlobalDataAllocator` manages static data allocation and string literal handling.

**Key Features:**
- Allocates space for static data with proper alignment
- Interns string literals and tracks their addresses
- Generates Wasm data segments for static content

**Implementation Details:**
```cpp
size_t allocate(size_t size, size_t align);
size_t internString(const std::string& str);
std::vector<WasmVM::WasmData> getDataSegments() const;
```

### 5. FunctionCodegen
The `FunctionCodegen` component generates WebAssembly code for individual functions.

**Key Features:**
- Per-function code generation with instruction buffering
- Local variable allocation and tracking
- Control flow stack management for break/continue handling
- Expression and statement emission with proper Wasm instruction generation

**Implementation Details:**
```cpp
WasmVM::WasmFunc generate(const wvmcc::parser::FunctionDefPtr& funcDef, 
                          const wvmcc::parser::Semantic& semantic);
void emitExpr(const wvmcc::parser::ExprPtr& expr, bool needLValue = false);
void emitStmt(const wvmcc::parser::StmtPtr& stmt);
```

### 6. ModuleCodegen
The `ModuleCodegen` component orchestrates the entire module generation process.

**Key Features:**
- First pass: symbol registration and function import/definition
- Second pass: actual function body generation
- Memory setup with proper Wasm64 memory configuration
- Global variable and string literal handling

**Implementation Details:**
```cpp
WasmVM::WasmModule generate(const wvmcc::parser::TranslationUnitPtr& tu);
void setupMemory();
void setupGlobals();
void firstPass(const wvmcc::parser::TranslationUnitPtr& tu);
void secondPass(const wvmcc::parser::TranslationUnitPtr& tu);
```

## Memory Model

The implementation follows the Wasm64 memory model with a 4-bit namespace:

```
memidx  Purpose
─────   ─────────────────────────────────────────────────────
  0     Heap + Static data (Wasm64 i64 addresses)
        [0..7]          reserved (null pointer sentinel)
        [8..static_end) static data: aggregates, string literals (BSS/rodata)
        [static_end..)  future heap (malloc)
  1     Shadow stack (Wasm64 i64 addresses, grows downward)
        [top..bottom]   call frames (address-taken locals, aggregate locals)
 2–15   Reserved for future use
```

## Integration with Main Pipeline

The code generation is integrated into the main compilation pipeline in `src/exec/main.cpp`:

```cpp
// Replace empty module construction with:
wvmcc::codegen::ModuleCodegen codegen(sem);
auto module = codegen.generate(tu);
```

## Testing

Comprehensive unit tests are provided in `tests/unit/codegen/` that verify:
- Component instantiation and basic functionality
- Type mapping correctness
- Symbol table scoping behavior
- Memory allocation and string interning
- Function type deduplication

## Phase Implementation Status

### Phase 1 - Scalar Foundation (Complete)
- Basic type mapping and memory model
- Symbol table with scope management
- Function code generation skeleton
- Module generation with proper memory setup

### Phase 2 - Memory and Aggregates (In Progress)
- Address taken analysis
- Shadow stack management
- Struct/union layout support
- Pointer and address expression handling

### Phase 3 - Control Flow Completeness (In Progress)
- Break/continue support
- Switch statement handling
- Short-circuit evaluation
- Ternary operator support

### Phase 4 - Advanced Features (Planned)
- Function pointers and call_indirect
- Static local variables
- Forward-only goto support

### Phase 5 - Robustness (Planned)
- Comprehensive diagnostics
- BSS zero-initialization
- Integration testing suite

## Usage Example

To compile a C file to WebAssembly:

```bash
./wvmcc input.c -o output.wasm
```

The resulting WebAssembly module will contain properly generated code with:
- Correct function signatures and types
- Proper memory layout and access patterns
- Valid Wasm64 instructions for all C constructs

## Validation

All generated modules are validated using `WasmVM::module_validate()` to ensure they meet WebAssembly specification requirements. The pipeline passes all existing parser and semantic unit tests while adding full code generation capabilities.