#pragma once

#include <unordered_map>
#include <vector>
#include <WasmVM.hpp>

namespace wvmcc::codegen {

class TypeIndexCache {
public:
    // Deduplicate and intern function types
    WasmVM::index_t intern(const WasmVM::FuncType& funcType);
    
    // Get the index for a previously interned function type
    std::optional<WasmVM::index_t> getIndex(const WasmVM::FuncType& funcType) const;
    
    // Get the total number of interned types
    size_t size() const { return types_.size(); }
    
private:
    // Map to store function types and their indices
    std::unordered_map<WasmVM::FuncType, WasmVM::index_t> typeMap_;
    
    // Vector to store the actual types in order
    std::vector<WasmVM::FuncType> types_;
};

} // namespace wvmcc::codegen