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
    
    // Compute field offsets and alignment
    size_t currentOffset = 0;
    size_t maxAlignment = 1;
    
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
                // Simple type - compute size based on the simple type specifiers
                for (const auto& simpleType : typeSpec.simple) {
                    switch (simpleType) {
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void:
                            memberSize = 0; // Void has no size, but this shouldn't happen in a struct member
                            memberAlignment = 1;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Char:
                            memberSize = 1;
                            memberAlignment = 1;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Short:
                            memberSize = 2;
                            memberAlignment = 2;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int:
                            memberSize = 4;
                            memberAlignment = 4;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Long:
                            memberSize = 8;
                            memberAlignment = 8;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Float:
                            memberSize = 4;
                            memberAlignment = 4;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Double:
                            memberSize = 8;
                            memberAlignment = 8;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Bool:
                            memberSize = 1;
                            memberAlignment = 1;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Complex:
                            memberSize = 0; // Complex types are not supported in this simplified version
                            memberAlignment = 1;
                            break;
                        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Imaginary:
                            memberSize = 0; // Imaginary types are not supported in this simplified version
                            memberAlignment = 1;
                            break;
                        default:
                            // For other types, assume size of 8 (pointer size)
                            memberSize = 8;
                            memberAlignment = 8;
                    }
                }
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
        
        // Apply alignment rules
        size_t alignedOffset = (currentOffset + memberAlignment - 1) & ~(memberAlignment - 1);
        currentOffset = alignedOffset;
        
        // Store field offset
        if (!member.declarators.empty() && member.declarators[0].declarator) {
            // Get the name of the field (if available)
            layout.fieldOffsets.emplace_back(member.declarators[0].declarator->id.name, currentOffset);
        } else {
            // Anonymous field or unnamed member
            layout.fieldOffsets.emplace_back("", currentOffset);
        }
        
        // Update max alignment and size
        layout.byteAlignment = std::max(layout.byteAlignment, memberAlignment);
        currentOffset += memberSize;
    }
    
    // Final alignment adjustment for the entire struct
    size_t finalAlignment = layout.byteAlignment;
    layout.byteSize = (currentOffset + finalAlignment - 1) & ~(finalAlignment - 1);
    
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