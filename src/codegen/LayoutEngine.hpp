#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include "../parser/AST.hpp"

namespace wvmcc::codegen {

struct StructLayout {
    size_t byteSize;
    size_t byteAlignment;
    std::vector<std::pair<std::string, size_t>> fieldOffsets; // name -> offset
};

class LayoutEngine {
public:
    // Compute layout for a struct or union
    StructLayout computeLayout(const wvmcc::parser::StructOrUnionSpecifier& structSpec);
    
    // Get cached layout for a struct/union (if already computed)
    std::optional<StructLayout> getCachedLayout(const wvmcc::parser::StructOrUnionSpecifier* structSpec);
    
    // Cache a computed layout
    void cacheLayout(const wvmcc::parser::StructOrUnionSpecifier* structSpec, const StructLayout& layout);

private:
    // Map from struct/union specifiers to their computed layouts
    std::unordered_map<const wvmcc::parser::StructOrUnionSpecifier*, StructLayout> layoutCache_;
    
    // Helper to compute field offsets with proper alignment
    size_t computeFieldOffset(const std::vector<wvmcc::parser::StructMember>& members, 
                              size_t currentOffset, 
                              const wvmcc::parser::StructMember& member,
                              size_t* maxAlignment);
};

} // namespace wvmcc::codegen