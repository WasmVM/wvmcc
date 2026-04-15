// Simple test file for codegen components - no GTest dependencies
#include <iostream>
#include <cassert>
#include "../../../src/codegen/TypeMap.hpp"
#include "../../../src/codegen/SymbolTable.hpp"
#include "../../../src/codegen/TypeIndexCache.hpp"
#include "../../../src/codegen/GlobalDataAllocator.hpp"

// Test that all codegen components can be instantiated and used
void test_codegen_components() {
    // Test TypeMap instantiation
    wvmcc::codegen::TypeMap typeMap;
    
    // Test SymbolTable instantiation
    wvmcc::codegen::SymbolTable symbolTable;
    
    // Test TypeIndexCache instantiation
    wvmcc::codegen::TypeIndexCache typeIndexCache;
    
    // Test GlobalDataAllocator instantiation
    wvmcc::codegen::GlobalDataAllocator dataAllocator;
    
    std::cout << "All codegen components instantiated successfully" << std::endl;
}

// Test TypeMap functionality with various types
void test_type_map() {
    wvmcc::codegen::TypeMap typeMap;
    
    // Create a simple type node for testing
    auto typeNode = wvmcc::parser::make_ast_with_span<wvmcc::parser::TypeNode>(wvmcc::SourceSpan{0, 0});
    typeNode->kind = wvmcc::parser::TypeNode::Kind::Builtin;
    
    // Test basic type mapping for different types
    auto wasmType = typeMap.toWasmType(typeNode);
    assert(wasmType != WasmVM::ValueType::none);
    
    // Test byte size calculation
    size_t size = typeMap.byteSize(typeNode);
    assert(size == 0); // Default for null type
    
    // Test byte alignment calculation
    size_t align = typeMap.byteAlignment(typeNode);
    assert(align == 1); // Default for null type
    
    // Test memory resident check
    bool isMemResident = typeMap.isMemoryResident(typeNode);
    assert(!isMemResident); // Default for null type
    
    std::cout << "TypeMap functionality test passed" << std::endl;
}

// Test SymbolTable functionality
void test_symbol_table() {
    wvmcc::codegen::SymbolTable symbolTable;
    
    // Test scope management
    symbolTable.pushScope();
    symbolTable.popScope();
    
    // Test symbol definition and lookup
    wvmcc::codegen::ScalarLocal localInfo;
    localInfo.type = nullptr;
    localInfo.isAddressTaken = false;
    localInfo.localIndex = 0;
    
    bool defined = symbolTable.define("test_var", localInfo);
    assert(defined);
    
    auto lookupResult = symbolTable.lookup("test_var");
    assert(lookupResult.has_value());
    
    // Test exists method
    bool exists = symbolTable.exists("test_var");
    assert(exists);
    
    std::cout << "SymbolTable functionality test passed" << std::endl;
}

// Test TypeIndexCache functionality
void test_type_index_cache() {
    wvmcc::codegen::TypeIndexCache typeIndexCache;
    
    // Create a simple function type
    WasmVM::FuncType funcType;
    funcType.params = {};
    funcType.results = {WasmVM::ValueType::i32};
    
    // Test interning
    auto index1 = typeIndexCache.intern(funcType);
    auto index2 = typeIndexCache.intern(funcType);
    
    // Should return the same index for identical types
    assert(index1 == index2);
    
    // Should have one type in cache
    assert(typeIndexCache.size() == 1);
    
    // Test getting index for non-existent type
    WasmVM::FuncType funcType2;
    funcType2.params = {};
    funcType2.results = {WasmVM::ValueType::i64};
    
    auto index3 = typeIndexCache.getIndex(funcType2);
    assert(!index3.has_value()); // Should not find this type
    
    // Test getting index for existing type
    auto index4 = typeIndexCache.getIndex(funcType);
    assert(index4.has_value());
    assert(*index4 == 0); // Should be the first (and only) index
    
    std::cout << "TypeIndexCache functionality test passed" << std::endl;
}

// Test GlobalDataAllocator functionality
void test_global_data_allocator() {
    wvmcc::codegen::GlobalDataAllocator dataAllocator;
    
    // Test basic allocation
    size_t addr1 = dataAllocator.allocate(8, 1);
    size_t addr2 = dataAllocator.allocate(4, 1);
    
    // Should get different addresses (aligned)
    assert(addr1 != addr2);
    
    // Test string interning
    size_t strAddr = dataAllocator.internString("hello");
    assert(strAddr != 0);
    
    // Test that same string returns same address
    size_t strAddr2 = dataAllocator.internString("hello");
    assert(strAddr == strAddr2);
    
    // Test that different string returns different address
    size_t strAddr3 = dataAllocator.internString("world");
    assert(strAddr != strAddr3);
    
    std::cout << "GlobalDataAllocator functionality test passed" << std::endl;
}

// Main test function
int main() {
    std::cout << "Running basic codegen tests..." << std::endl;
    
    test_codegen_components();
    test_type_map();
    test_symbol_table();
    test_type_index_cache();
    test_global_data_allocator();
    
    std::cout << "All basic codegen tests passed!" << std::endl;
    return 0;
}