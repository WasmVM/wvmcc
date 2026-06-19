#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include "../parser/AST.hpp"

namespace wvmcc::codegen {

// Per-field bit-field placement within its storage unit. byteOffset (in
// StructLayout::fieldOffsets) is the unit's offset; these describe the field's
// position inside it. LSB-first: bitOffset 0 is the least-significant bit.
struct BitFieldInfo {
    bool isBitfield = false;
    unsigned bitOffset = 0;   // first bit within the storage unit
    unsigned bitWidth = 0;    // field width in bits
    size_t storageSize = 0;   // access-unit size in bytes (1/2/4/8)
    bool isSigned = false;    // signed bit-fields sign-extend on load
};

struct StructLayout {
    size_t byteSize;
    size_t byteAlignment;
    std::vector<std::pair<std::string, size_t>> fieldOffsets; // name -> offset
    std::vector<std::pair<std::string, BitFieldInfo>> bitFields; // name -> bit info
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