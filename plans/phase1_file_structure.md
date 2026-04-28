# Phase 1 File Structure

## Directory Creation
- [ ] Create `src/codegen/` directory

## Header Files to Create
1. `src/codegen/TypeMap.hpp`
2. `src/codegen/SymbolTable.hpp` 
3. `src/codegen/TypeIndexCache.hpp`
4. `src/codegen/GlobalDataAllocator.hpp`
5. `src/codegen/FunctionCodegen.hpp`
6. `src/codegen/ModuleCodegen.hpp`

## Implementation Files to Create
1. `src/codegen/TypeMap.cpp`
2. `src/codegen/SymbolTable.cpp`
3. `src/codegen/TypeIndexCache.cpp`
4. `src/codegen/GlobalDataAllocator.cpp`
5. `src/codegen/FunctionCodegen.cpp`
6. `src/codegen/ModuleCodegen.cpp`

## Integration Points
- `src/exec/main.cpp` (lines 207-208)
- `CMakeLists.txt` (add codegen source glob)

## Dependencies
- `WasmVM/src/include/structures/WasmInstr.hpp` - All instruction types
- `WasmVM/src/include/structures/WasmModule.hpp` - Module struct
- `WasmVM/src/include/structures/WasmFunc.hpp` - Function struct
- `WasmVM/src/include/Types.hpp` - `ValueType`, `FuncType`, `index_t`
- `src/parser/AST.hpp` - AST types used throughout
- `src/parser/Semantic.hpp` - `typeOfExpr()`, `buildTypeFromDeclaration()`