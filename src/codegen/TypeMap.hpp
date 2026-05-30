#pragma once

#include "LayoutEngine.hpp"
#include "../parser/AST.hpp"
#include <WasmVM.hpp>

namespace wvmcc::parser { class Semantic; }

namespace wvmcc::codegen {

class TypeMap {
public:
    // Optional semantic context used to resolve typedef-name member types
    // (e.g. `FILE *`, `size_t`) in getFieldType. When unset, getFieldType
    // falls back to a typedef-unaware reconstruction.
    void setSemantic(const wvmcc::parser::Semantic* s) { semantic_ = s; }

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

    // Get the byte offset of a named field in a struct/union type
    size_t getFieldOffset(const wvmcc::parser::TypeNodePtr& type, const std::string& fieldName) const;

    // Get the TypeNode of a named field in a struct/union type (nullptr if not found)
    wvmcc::parser::TypeNodePtr getFieldType(const wvmcc::parser::TypeNodePtr& type, const std::string& fieldName) const;

    // Get the named fields of a struct/union in declaration order (empty if not
    // a struct/union or no members). Used to encode aggregate initializers.
    std::vector<std::string> getOrderedFieldNames(const wvmcc::parser::TypeNodePtr& type) const;
    
private:
    // Helper to get the base type for a node
    WasmVM::ValueType getBaseType(const wvmcc::parser::TypeNodePtr& type) const;

    // Whether an integer scalar is unsigned (controls narrow-load extension)
    bool isUnsignedScalarInteger(const wvmcc::parser::TypeNodePtr& type) const;
    
    // Helper to get size for a simple type
    size_t getSimpleTypeSize(const wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier& simpleType) const;
    
    // Helper to get alignment for a simple type
    size_t getSimpleTypeAlignment(const wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier& simpleType) const;
    
    mutable LayoutEngine layoutEngine_;
    const wvmcc::parser::Semantic* semantic_ = nullptr;
};

} // namespace wvmcc::codegen