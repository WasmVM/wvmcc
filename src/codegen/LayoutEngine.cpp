#include "LayoutEngine.hpp"
#include <algorithm>
#include <cassert>

namespace wvmcc::codegen {

// Walk a declarator chain to its Identifier node and return the declared
// name. Chain shape (outer→inner): [Array/Function suffixes] → Identifier
// → [leading `*`s].
static std::string declaratorName(const wvmcc::parser::DeclaratorPtr& decl) {
    for (auto cur = decl; cur; cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (cur->kind == wvmcc::parser::Declarator::Kind::Identifier
            || !cur->id.name.empty())
            return cur->id.name;
    }
    return std::string();
}

// Best-effort constant fold of an array-size expression (integer literal).
static size_t arrayCount(const wvmcc::parser::Declarator* arr) {
    if (arr->array.size.has_value() && *arr->array.size) {
        const auto& e = **arr->array.size;
        if (e.kind == wvmcc::parser::Expr::Kind::Integer) {
            auto v = static_cast<const wvmcc::parser::IntegerLiteral&>(e).value;
            if (v > 0) return static_cast<size_t>(v);
        }
    }
    return 1; // unknown / flexible array → treat as one element
}

// Given the scalar base size/alignment from the specifiers, apply the
// pointer/array layers of a member's declarator to obtain the member's
// in-memory size and alignment. Pointers are 8/8 (wasm64); arrays multiply
// the element size by the element count.
static void applyDeclaratorToLayout(const wvmcc::parser::DeclaratorPtr& decl,
                                    size_t& memberSize, size_t& memberAlignment) {
    if (!decl) return;
    // Collect layers in type-application order (innermost → outermost),
    // matching TypeMap::applyDeclaratorLayers: leading `*`s (reversed) first,
    // then trailing array/function suffixes.
    std::vector<const wvmcc::parser::Declarator*> belowId, aboveId;
    bool sawId = false;
    for (auto cur = decl; cur; cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (cur->kind == wvmcc::parser::Declarator::Kind::Identifier) { sawId = true; continue; }
        if (cur->kind == wvmcc::parser::Declarator::Kind::Pointer
            || cur->kind == wvmcc::parser::Declarator::Kind::Array) {
            (sawId ? belowId : aboveId).push_back(cur.get());
        }
    }
    auto step = [&](const wvmcc::parser::Declarator* d) {
        if (d->kind == wvmcc::parser::Declarator::Kind::Pointer) {
            memberSize = 8; memberAlignment = 8;
        } else { // Array
            memberSize *= arrayCount(d);
        }
    };
    for (auto it = belowId.rbegin(); it != belowId.rend(); ++it) step(*it);
    for (auto* d : aboveId) step(d);
}

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
        
        // Place one field, applying alignment and advancing the running offset
        // (struct) or overlaying at 0 (union).
        auto placeField = [&](const std::string& name, size_t sz, size_t al) {
            size_t fieldOffset;
            if (isUnion) {
                fieldOffset = 0;
                maxMemberSize = std::max(maxMemberSize, sz);
            } else {
                size_t alignedOffset = (currentOffset + al - 1) & ~(al - 1);
                fieldOffset = alignedOffset;
                currentOffset = alignedOffset + sz;
            }
            layout.fieldOffsets.emplace_back(name, fieldOffset);
            layout.byteAlignment = std::max(layout.byteAlignment, al);
        };

        // A single member declaration may declare several fields
        // (`int a, b, c;`, or mixed `int x, *p, arr[4];`). Each declarator
        // shares the member's base type but carries its own pointer/array
        // adornment and gets its own offset. (Previously only declarators[0]
        // was laid out, so `int a, b;` aliased every field onto offset 0 and
        // under-sized the struct.)
        if (member.declarators.empty()) {
            // Anonymous member (e.g. an anonymous struct/union).
            placeField("", memberSize, memberAlignment);
        } else {
            for (const auto& sd : member.declarators) {
                size_t sz = memberSize, al = memberAlignment;
                if (sd.declarator) applyDeclaratorToLayout(sd.declarator, sz, al);
                placeField(sd.declarator ? declaratorName(sd.declarator) : "", sz, al);
            }
        }
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