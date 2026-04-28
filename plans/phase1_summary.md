# Phase 1: Scalar Foundation Implementation Summary

## Objective
Compile `int add(int a, int b) { return a+b; }` to valid `.wasm` with the following capabilities:
- Scalar local variables (int/long/pointer types)
- Basic arithmetic operations
- Control flow (return, if/else, while, for)
- Function calls
- Basic expression evaluation

## Key Components to Implement

### 1. Infrastructure Layer
- **TypeMap**: Maps C types to Wasm types with proper size/alignment handling
- **SymbolTable**: Manages symbol scope and resolution for locals and globals
- **TypeIndexCache**: Deduplicates function types for efficient module generation
- **GlobalDataAllocator**: Handles static data allocation and string literal management

### 2. Code Generation Layer
- **ModuleCodegen**: Top-level driver that orchestrates module creation
- **FunctionCodegen**: Per-function code generator that produces WasmFunc structures

### 3. Expression and Statement Handling
- **Expression Lowering**: Convert C expressions to Wasm instructions
- **Statement Lowering**: Convert C statements to Wasm control flow

## Implementation Approach

### Phase 1.1: Infrastructure Setup
1. Create codegen directory and header files
2. Implement TypeMap with Wasm64 type mapping rules
3. Implement SymbolTable with scope management
4. Implement TypeIndexCache for function type deduplication
5. Implement GlobalDataAllocator for static data management

### Phase 1.2: Module Generation Skeleton
1. Implement ModuleCodegen with basic module structure
2. Set up memory types (mem[0] for heap/static, mem[1] for shadow stack)
3. Initialize __stack_pointer global
4. Implement first pass (symbol registration)
5. Implement second pass (placeholder function bodies)

### Phase 1.3: Function Code Generation
1. Implement FunctionCodegen with local allocation and instruction emission
2. Wire function generation into module codegen
3. Integrate with main.cpp for module generation
4. Ensure compilation works with empty function bodies

### Phase 1.4: Expression Handling
1. Implement scalar expression lowering
2. Handle integer/char literals
3. Handle identifier access (locals/globals)
4. Implement arithmetic and comparison operations
5. Implement unary operations and casts

### Phase 1.5: Statement Handling
1. Implement basic control flow statements
2. Handle return, expression, compound statements
3. Implement if/else, while, for constructs
4. Handle declaration block items

### Phase 1.6: Function Calls
1. Implement direct function call handling
2. Verify type matching for call sites
3. Support external function imports

### Phase 1.7: Verification
1. Compile test case: `int add(int a, int b) { return a+b; }`
2. Validate module with module_validate()
3. Inspect output with readwasm
4. Run existing unit tests
5. Test extern import functionality

## Expected Output
A valid `.wasm` file that:
- Passes `module_validate()`
- Has correct function signature and body
- Can be executed via `wasmvm`
- Passes all existing parser/semantic unit tests