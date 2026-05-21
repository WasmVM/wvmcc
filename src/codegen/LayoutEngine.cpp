#include "LayoutEngine.hpp"
#include <algorithm>
#include <cassert>

namespace wvmcc::codegen {

StructLayout LayoutEngine::computeLayout(const wvmcc::parser::StructOrUnionSpecifier& structSpec) {
    // Check if we've already computed this layout
    auto cached = getCachedLayout(&structSpec);
    if (cached.has_value()) {
        return cached.value();
    }
    
    StructLayout layout;
    layout.byteSize = 0;
    layout.byteAlignment = 1;
    layout.fieldOffsets.clear();
    
    if (structSpec.members.empty()) {
        // Empty struct/union - size is 0, alignment is 1
        layout.byteSize = 0;
        layout.byteAlignment = 1;
        return layout;
    }
    
    // Compute field offsets and alignment. Unions overlay all members at
    // offset 0; structs lay them out sequentially with alignment padding.
    const bool isUnion = (structSpec.kind == wvmcc::parser::StructOrUnionSpecifier::Kind::Union);
    size_t currentOffset = 0;
    size_t maxMemberSize = 0;

    for (const auto& member : structSpec.members) {
        // Get the type of this member
        const auto& specifiers = member.specifiers;
        
        // For now, we'll compute the size and alignment of this member's type
        // In a more complete implementation, we'd need to handle complex types
        
        // For simplicity in this initial implementation, we'll compute the size
        // based on the member's type and apply proper alignment rules
        
        // Compute the size of this member (this is a simplified approach)
        size_t memberSize = 0;
        size_t memberAlignment = 1;
        
        // Handle simple types (int, char, etc.)
        if (!specifiers.typeSpecifiers.empty()) {
            const auto& typeSpec = specifiers.typeSpecifiers[0];
            
            if (typeSpec.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
                bool hasDouble = false, hasFloat = false, hasLong = false, hasShort = false;
                bool hasChar = false, hasBool = false;
                for (auto s : typeSpec.simple) {
                    if (s == STS::Double) hasDouble = true;
                    else if (s == STS::Float) hasFloat = true;
                    else if (s == STS::Long)  hasLong  = true;
                    else if (s == STS::Short) hasShort = true;
                    else if (s == STS::Char)  hasChar  = true;
                    else if (s == STS::Bool)  hasBool  = true;
                }
                if (hasDouble)      { memberSize = 8; memberAlignment = 8; } // long double → double
                else if (hasFloat)  { memberSize = 4; memberAlignment = 4; }
                else if (hasLong)   { memberSize = 8; memberAlignment = 8; }
                else if (hasShort)  { memberSize = 2; memberAlignment = 2; }
                else if (hasChar || hasBool) { memberSize = 1; memberAlignment = 1; }
                else                { memberSize = 4; memberAlignment = 4; } // int / signed / unsigned
            } else if (typeSpec.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion) {
                // Struct or union member - recursively compute layout
                if (typeSpec.su) {
                    // For now, we'll treat nested structs/unions as 8 bytes (pointer size)
                    memberSize = 8;
                    memberAlignment = 8;
                } else {
                    // Fallback for unknown types
                    memberSize = 8;
                    memberAlignment = 8;
                }
            } else {
                // Fallback for other type specifiers
                memberSize = 8;
                memberAlignment = 8;
            }
        } else {
            // Fallback for unknown types
            memberSize = 8;
            memberAlignment = 8;
        }
        
        size_t fieldOffset;
        if (isUnion) {
            // All union members overlay at offset 0.
            fieldOffset = 0;
            maxMemberSize = std::max(maxMemberSize, memberSize);
        } else {
            size_t alignedOffset = (currentOffset + memberAlignment - 1) & ~(memberAlignment - 1);
            currentOffset = alignedOffset;
            fieldOffset = currentOffset;
            currentOffset += memberSize;
        }

        if (!member.declarators.empty() && member.declarators[0].declarator) {
            layout.fieldOffsets.emplace_back(member.declarators[0].declarator->id.name, fieldOffset);
        } else {
            layout.fieldOffsets.emplace_back("", fieldOffset);
        }

        // Update max alignment.
        layout.byteAlignment = std::max(layout.byteAlignment, memberAlignment);
    }

    // Final size: structs pad to alignment; unions are max-member-size padded.
    size_t finalAlignment = layout.byteAlignment;
    size_t rawSize = isUnion ? maxMemberSize : currentOffset;
    layout.byteSize = (rawSize + finalAlignment - 1) & ~(finalAlignment - 1);
    
    // Cache the result
    cacheLayout(&structSpec, layout);
    
    return layout;
}

std::optional<StructLayout> LayoutEngine::getCachedLayout(const wvmcc::parser::StructOrUnionSpecifier* structSpec) {
    if (structSpec == nullptr) {
        return std::nullopt;
    }
    
    auto it = layoutCache_.find(structSpec);
    if (it != layoutCache_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

void LayoutEngine::cacheLayout(const wvmcc::parser::StructOrUnionSpecifier* structSpec, const StructLayout& layout) {
    if (structSpec != nullptr) {
        layoutCache_[structSpec] = layout;
    }
}

size_t LayoutEngine::computeFieldOffset(const std::vector<wvmcc::parser::StructMember>& members, 
                                        size_t currentOffset, 
                                        const wvmcc::parser::StructMember& member,
                                        size_t* maxAlignment) {
    // This is a simplified implementation - in a real system, this would be more complex
    // and handle bit-fields, padding, etc.
    
    // For now, we'll just return the current offset (this is a placeholder)
    return currentOffset;
}

} // namespace wvmcc::codegen