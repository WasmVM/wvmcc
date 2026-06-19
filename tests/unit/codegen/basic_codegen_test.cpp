// Simple test file for codegen components - no GTest dependencies
#include <iostream>
#include <cassert>
#include "../../../src/codegen/FunctionCodegen.hpp"
#include "../../../src/codegen/TypeMap.hpp"
#include "../../../src/codegen/SymbolTable.hpp"
#include "../../../src/codegen/TypeIndexCache.hpp"
#include "../../../src/codegen/GlobalDataAllocator.hpp"
#include "../../../src/codegen/LayoutEngine.hpp"
#include "../../../src/parser/AST.hpp"

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
    
    // Helper to create a builtin type node
    auto makeBuiltinType = [](wvmcc::parser::TypeNode::Kind kind, const std::vector<wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier>& specs) {
        auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        node->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        node->simple = specs;
        return node;
    };

    // Test int -> i32
    auto typeInt = makeBuiltinType(wvmcc::parser::TypeNode::Kind::Builtin, {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int});
    assert(typeMap.toWasmType(typeInt) == WasmVM::ValueType::i32);
    assert(typeMap.byteSize(typeInt) == 4);
    assert(typeMap.byteAlignment(typeInt) == 4);

    // Test long -> i64
    auto typeLong = makeBuiltinType(wvmcc::parser::TypeNode::Kind::Builtin, {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Long});
    assert(typeMap.toWasmType(typeLong) == WasmVM::ValueType::i64);
    assert(typeMap.byteSize(typeLong) == 8);
    assert(typeMap.byteAlignment(typeLong) == 8);

    // Test float -> f32
    auto typeFloat = makeBuiltinType(wvmcc::parser::TypeNode::Kind::Builtin, {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Float});
    assert(typeMap.toWasmType(typeFloat) == WasmVM::ValueType::f32);
    assert(typeMap.byteSize(typeFloat) == 4);

    // Test double -> f64
    auto typeDouble = makeBuiltinType(wvmcc::parser::TypeNode::Kind::Builtin, {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Double});
    assert(typeMap.toWasmType(typeDouble) == WasmVM::ValueType::f64);
    assert(typeMap.byteSize(typeDouble) == 8);

    // Test char -> i32 (promoted in Wasm)
    auto typeChar = makeBuiltinType(wvmcc::parser::TypeNode::Kind::Builtin, {wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Char});
    assert(typeMap.toWasmType(typeChar) == WasmVM::ValueType::i32);
    assert(typeMap.byteSize(typeChar) == 1);
    
    std::cout << "TypeMap functionality test passed" << std::endl;
}

// Test SymbolTable functionality
void test_symbol_table() {
    wvmcc::codegen::SymbolTable symbolTable;
    
    // Test scope management
    symbolTable.pushScope();
    symbolTable.popScope();

    // Test symbol definition and lookup (requires an active scope)
    symbolTable.pushScope();
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

    // Test scope and shadowing
    symbolTable.pushScope();
    wvmcc::codegen::ScalarLocal innerLocal;
    innerLocal.type = nullptr;
    innerLocal.isAddressTaken = false;
    innerLocal.localIndex = 1;
    symbolTable.define("test_var", innerLocal); // Shadowing

    auto shadowedResult = symbolTable.lookup("test_var");
    assert(shadowedResult.has_value());
    assert(std::get<wvmcc::codegen::ScalarLocal>(*shadowedResult).localIndex == 1);

    symbolTable.popScope();
    auto restoredResult = symbolTable.lookup("test_var");
    assert(restoredResult.has_value());
    assert(std::get<wvmcc::codegen::ScalarLocal>(*restoredResult).localIndex == 0);
    
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

    // Test basic allocation and alignment
    size_t addr1 = dataAllocator.allocate(8, 8);
    assert(addr1 % 8 == 0);

    size_t addr2 = dataAllocator.allocate(4, 4);
    assert(addr2 % 4 == 0);
    assert(addr1 != addr2);

    // Test larger alignment
    size_t addr3 = dataAllocator.allocate(1, 64);
    assert(addr3 % 64 == 0);

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

void test_get_data_segments() {
    wvmcc::codegen::GlobalDataAllocator dataAllocator;

    size_t helloAddr = dataAllocator.internString("hello");

    auto segments = dataAllocator.getDataSegments();
    assert(segments.size() == 1);

    const auto& seg = segments[0];
    assert(seg.mode.type == WasmVM::WasmData::DataMode::Mode::active);
    assert(seg.mode.memidx.has_value() && *seg.mode.memidx == 0);
    assert(seg.mode.offset.has_value());

    auto* offset = std::get_if<WasmVM::Instr::I64_const>(&*seg.mode.offset);
    assert(offset != nullptr);
    assert((size_t)offset->value == helloAddr);

    // "hello\0" = 68 65 6c 6c 6f 00
    assert(seg.init.size() == 6);
    assert(seg.init[0] == std::byte(0x68));
    assert(seg.init[1] == std::byte(0x65));
    assert(seg.init[2] == std::byte(0x6c));
    assert(seg.init[3] == std::byte(0x6c));
    assert(seg.init[4] == std::byte(0x6f));
    assert(seg.init[5] == std::byte(0x00));

    // Deduplication: same address, still one segment
    dataAllocator.internString("hello");
    auto segments2 = dataAllocator.getDataSegments();
    assert(segments2.size() == 1);

    // Second distinct string produces a second segment
    dataAllocator.internString("world");
    auto segments3 = dataAllocator.getDataSegments();
    assert(segments3.size() == 2);

    std::cout << "getDataSegments test passed" << std::endl;
}

// Test FunctionCodegen expression emission
void test_function_codegen_expressions() {
    wvmcc::codegen::TypeMap typeMap;
    wvmcc::codegen::SymbolTable symbolTable;
    wvmcc::codegen::FunctionCodegen codegen(typeMap, symbolTable);

    // Test Integer Literal emission (i32)
    auto intLit = wvmcc::parser::make_ast<wvmcc::parser::IntegerLiteral>();
    intLit->value = 42;
    intLit->raw = "42";
    
    // We need to wrap it in a shared_ptr for emitExpr
    auto intLitPtr = std::make_shared<wvmcc::parser::IntegerLiteral>(*intLit);
    
}

// Regression: a struct member declaration with multiple declarators
// (`int a, b, c;`) must lay out every field, not just the first. Previously
// only declarators[0] was placed, so b/c aliased onto offset 0 and the struct
// was under-sized (sizeof == 4 for three ints).
void test_struct_multi_declarator_layout() {
    using namespace wvmcc::parser;
    using STS = DeclarationSpecifiers::SimpleTypeSpecifier;

    StructOrUnionSpecifier spec;
    spec.kind = StructOrUnionSpecifier::Kind::Struct;
    spec.hasBody = true;

    StructMember m;
    DeclarationSpecifiers::TypeSpecifier ts;
    ts.kind = DeclarationSpecifiers::TypeSpecifier::Kind::Simple;
    ts.simple = {STS::Int};
    m.specifiers.typeSpecifiers.push_back(ts);
    for (const char* nm : {"a", "b", "c"}) {
        StructDeclarator sd;
        sd.declarator = make_ast<Declarator>();
        sd.declarator->kind = Declarator::Kind::Identifier;
        sd.declarator->id.name = nm;
        m.declarators.push_back(sd);
    }
    spec.members.push_back(std::move(m));

    wvmcc::codegen::LayoutEngine layout;
    auto L = layout.computeLayout(spec);
    assert(L.byteSize == 12);
    assert(L.byteAlignment == 4);
    assert(L.fieldOffsets.size() == 3);
    assert(L.fieldOffsets[0].first == "a" && L.fieldOffsets[0].second == 0);
    assert(L.fieldOffsets[1].first == "b" && L.fieldOffsets[1].second == 4);
    assert(L.fieldOffsets[2].first == "c" && L.fieldOffsets[2].second == 8);

    std::cout << "struct multi-declarator layout test passed" << std::endl;
}

// Main test function
int main() {
    std::cout << "Running basic codegen tests..." << std::endl;

    test_codegen_components();
    test_type_map();
    test_symbol_table();
    test_type_index_cache();
    test_global_data_allocator();
    test_get_data_segments();
    test_function_codegen_expressions();
    test_struct_multi_declarator_layout();

    std::cout << "All basic codegen tests passed!" << std::endl;
    return 0;
}