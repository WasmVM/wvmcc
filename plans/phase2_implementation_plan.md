# Phase 2 Implementation Plan

This document outlines the detailed implementation plan for completing Phase 2 of the wvmcc compiler, addressing the remaining tasks after the AddressTakenAnalyzer and LayoutEngine components have been created.

## 1. MemoryLocal Promotion Logic in FunctionCodegen

### Overview
The current implementation in `FunctionCodegen.cpp` only allocates local variables as `ScalarLocal` types. We need to modify this to promote address-taken variables to `MemoryLocal` instead of `ScalarLocal`.

### Implementation Steps
1. **Modify `allocLocal()` method** to accept an additional parameter indicating whether the variable is address-taken
2. **Update `generate()` method** to:
   - Run AddressTakenAnalyzer at the start to identify address-taken variables
   - Track which local variables are address-taken
   - Allocate appropriate local variable types (ScalarLocal vs MemoryLocal)
3. **Update symbol table handling** to properly store and retrieve MemoryLocal variables

### Key Changes
- Modify `FunctionCodegen::allocLocal()` to support address-taken variables
- Update `FunctionCodegen::generate()` to use AddressTakenAnalyzer results
- Modify symbol table lookups to handle MemoryLocal types

## 2. Prologue/Epilogue Generation for Functions with MemoryLocal Variables

### Overview
Functions that contain `MemoryLocal` variables need to generate proper prologue and epilogue code to manage stack frame layout.

### Implementation Steps
1. **Detect functions with MemoryLocal variables** by checking the AddressTakenAnalyzer results
2. **Generate prologue code** that:
   - Sets up stack frame (if needed)
   - Allocates space for local variables
3. **Generate epilogue code** that:
   - Handles cleanup of local variables
   - Ensures proper function exit

### Key Changes
- Modify `FunctionCodegen::generate()` to conditionally generate prologue/epilogue
- Add stack frame management logic for functions with MemoryLocal variables

## 3. MemoryLocal Expression Emission in `emitExpr`

### Overview
The current `emitExpr` implementation only handles `ScalarLocal` variables. We need to add support for `MemoryLocal` variables.

### Implementation Steps
1. **Modify `emitIdentifierExpr()`** to handle MemoryLocal variables:
   - For MemoryLocal, emit code that loads from frame offset instead of local index
2. **Add new methods** for handling memory expressions:
   - `emitAddressOfExpr()` for address-of operator (&)
   - `emitDereferenceExpr()` for pointer dereference (*)

### Key Changes
- Update `FunctionCodegen::emitIdentifierExpr()` to handle MemoryLocal case
- Add support for address-of operator in `emitUnaryExpr()`
- Add support for pointer dereference in `emitUnaryExpr()`

## 4. Pointer Arithmetic Support

### Overview
Implement support for pointer arithmetic operations (e.g., `ptr + offset`, `ptr - offset`).

### Implementation Steps
1. **Modify `emitBinaryExpr()`** to handle pointer arithmetic:
   - Detect when operands are pointers
   - Perform appropriate arithmetic with proper size calculations
2. **Add pointer comparison operations** (==, !=, <, >, etc.)

### Key Changes
- Extend `FunctionCodegen::emitBinaryExpr()` to handle pointer arithmetic
- Add proper type checking for pointer operations

## 5. Member Access and Array Indexing Expressions

### Overview
Implement support for accessing struct members (e.g., `struct_var.member`) and array elements (e.g., `array[index]`).

### Implementation Steps
1. **Add new expression emission methods**:
   - `emitMemberAccessExpr()` for member access
   - `emitArrayIndexExpr()` for array indexing
2. **Modify `emitExpr()`** to handle new expression types

### Key Changes
- Add `emitMemberAccessExpr()` method in `FunctionCodegen`
- Add `emitArrayIndexExpr()` method in `FunctionCodegen`
- Update `emitExpr()` to handle new expression types

## 6. String Literal Handling and Data Segment Emission

### Overview
Implement support for string literals, including storing them in data segments and generating appropriate references.

### Implementation Steps
1. **Modify `emitStringLiteral()`** in `FunctionCodegen`:
   - Handle string literal expressions
2. **Update `ModuleCodegen`** to:
   - Collect string literals from the translation unit
   - Emit them as data segments in the Wasm module
3. **Add string literal support** to symbol table and type system

### Key Changes
- Add `emitStringLiteral()` method in `FunctionCodegen`
- Modify `ModuleCodegen` to collect and emit string literals
- Update symbol table to handle string literal references

## 7. ModuleCodegen Data Segment Updates

### Overview
Update `ModuleCodegen` to properly emit data segments for strings and aggregates.

### Implementation Steps
1. **Modify `ModuleCodegen`** to:
   - Collect string literals and aggregate data
   - Create appropriate data segments in the Wasm module
2. **Add support for global memory variables** in symbol table

### Key Changes
- Add string literal collection in `ModuleCodegen::secondPass()`
- Update data segment emission logic
- Add support for global memory variables

## 8. Struct Parameter Handling (ABI Transformation)

### Overview
Implement ABI transformation for struct parameters, where structs are passed by reference.

### Implementation Steps
1. **Modify function signature handling**:
   - Detect when struct parameters are present
   - Transform them to pass by reference (pointer)
2. **Update function call handling**:
   - When calling functions with struct parameters, pass address instead of value

### Key Changes
- Modify function parameter processing logic
- Update function call emission to handle struct parameters

## 9. Struct Return Handling (Hidden First Parameter)

### Overview
Implement support for functions that return structs by modifying the function signature to include a hidden first parameter.

### Implementation Steps
1. **Modify function return handling**:
   - Detect functions that return structs
   - Transform them to use a hidden first parameter (pointer to return value)
2. **Update function call handling**:
   - When calling such functions, pass address of return value

### Key Changes
- Modify function signature generation for struct returns
- Update function call emission logic

## 10. CompoundLiteral Support

### Overview
Add support for compound literal expressions (e.g., `{1, 2, 3}`).

### Implementation Steps
1. **Add CompoundLiteral expression handling**:
   - Modify `emitExpr()` to handle compound literals
   - Add appropriate memory allocation and initialization

### Key Changes
- Add support for `CompoundLiteralExpr` in `emitExpr()`
- Implement compound literal emission logic

## Implementation Priority Order

1. MemoryLocal promotion and expression emission (core functionality)
2. Prologue/epilogue generation for functions with MemoryLocal variables
3. Pointer arithmetic support
4. Member access and array indexing expressions
5. String literal handling and data segment emission
6. ModuleCodegen data segment updates
7. Struct parameter handling (ABI transformation)
8. Struct return handling (hidden first parameter)
9. CompoundLiteral support

## Technical Considerations

### Memory Layout
- MemoryLocal variables will be stored at fixed offsets from the frame pointer
- Stack frame layout needs to account for alignment requirements

### Wasm Memory Operations
- Use `i32.load` and `i32.store` for memory access (assuming 32-bit addressing)
- Implement proper alignment checks and padding calculations
- Handle pointer arithmetic with appropriate size calculations

### Type System Integration
- Ensure proper type checking for all memory operations
- Handle type conversions between pointer and integer types

### Performance Considerations
- Minimize stack frame size by optimizing local variable layout
- Use efficient memory access patterns where possible

This plan provides a comprehensive roadmap for completing the remaining Phase 2 tasks, ensuring proper integration with existing components and maintaining code quality standards.