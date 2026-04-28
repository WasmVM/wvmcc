#include "TypeIndexCache.hpp"

namespace wvmcc::codegen {

WasmVM::index_t TypeIndexCache::intern(const WasmVM::FuncType& funcType) {
    // Check if this type is already cached
    auto it = typeMap_.find(funcType);
    if (it != typeMap_.end()) {
        return it->second;
    }
    
    // Add new type to the cache
    WasmVM::index_t index = static_cast<WasmVM::index_t>(types_.size());
    types_.push_back(funcType);
    typeMap_[funcType] = index;
    
    return index;
}

std::optional<WasmVM::index_t> TypeIndexCache::getIndex(const WasmVM::FuncType& funcType) const {
    auto it = typeMap_.find(funcType);
    if (it != typeMap_.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace wvmcc::codegen