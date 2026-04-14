#pragma once

#include "../parser/AST.hpp"
#include <WasmVM.hpp>

namespace wvmcc::codegen {

class TypeMap {
public:
    // Convert a C type node to Wasm type
    WasmVM::ValueType toWasmType(const wvmcc::parser::TypeNodePtr& type) const;
    
    // Get the byte size of a type
    size_t byteSize(const wvmcc::parser::TypeNodePtr& type) const;
    
    // Get the byte alignment of a type
    size_t byteAlignment(const wvmcc::parser::TypeNodePtr& type) const;
    
    // Check if a type is memory resident (struct, union, array)
    bool isMemoryResident(const wvmcc::parser::TypeNodePtr& type) const;
    
    // Create a load instruction for a type with specified memory index
    WasmVM::WasmInstr makeLoad(const wvmcc::parser::TypeNodePtr& type, uint8_t memidx) const;
    
    // Create a store instruction for a type with specified memory index
    WasmVM::WasmInstr makeStore(const wvmcc::parser::TypeNodePtr& type, uint8_t memidx) const;
    
private:
    // Helper to get the base type for a node
    WasmVM::ValueType getBaseType(const wvmcc::parser::TypeNodePtr& type) const;
    
    // Helper to get size for a simple type
    size_t getSimpleTypeSize(const wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier& simpleType) const;
    
    // Helper to get alignment for a simple type
    size_t getSimpleTypeAlignment(const wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier& simpleType) const;
};

} // namespace wvmcc::codegen