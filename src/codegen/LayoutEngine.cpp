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
    // `offsetBits` is the running bit cursor (LSB-first) so adjacent bit-fields
    // pack into a shared storage unit; ordinary fields round it up to a byte.
    const bool isUnion = (structSpec.kind == wvmcc::parser::StructOrUnionSpecifier::Kind::Union);
    size_t offsetBits = 0;
    size_t maxMemberSize = 0;

    for (const auto& member : structSpec.members) {
        // Get the type of this member
        const auto& specifiers = member.specifiers;

        // Compute the size, alignment and signedness of this member's base type.
        size_t memberSize = 0;
        size_t memberAlignment = 1;
        bool memberSigned = true;
        // For a struct/union base, the nested layout is computed lazily and only
        // for an *embedded* (non-pointer) member: a pointer-to-the-same-struct
        // member (`struct node *next`) must NOT recurse, or self-referential
        // types would loop forever.
        const wvmcc::parser::StructOrUnionSpecifier* structBaseSu = nullptr;

        if (!specifiers.typeSpecifiers.empty()) {
            const auto& typeSpec = specifiers.typeSpecifiers[0];

            if (typeSpec.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple) {
                using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
                bool hasDouble = false, hasFloat = false, hasLong = false, hasShort = false;
                bool hasChar = false, hasBool = false, hasUnsigned = false;
                for (auto s : typeSpec.simple) {
                    if (s == STS::Double) hasDouble = true;
                    else if (s == STS::Float) hasFloat = true;
                    else if (s == STS::Long)  hasLong  = true;
                    else if (s == STS::Short) hasShort = true;
                    else if (s == STS::Char)  hasChar  = true;
                    else if (s == STS::Bool)  hasBool  = true;
                    else if (s == STS::Unsigned) hasUnsigned = true;
                }
                if (hasDouble)      { memberSize = 8; memberAlignment = 8; } // long double → double
                else if (hasFloat)  { memberSize = 4; memberAlignment = 4; }
                else if (hasLong)   { memberSize = 8; memberAlignment = 8; }
                else if (hasShort)  { memberSize = 2; memberAlignment = 2; }
                else if (hasChar || hasBool) { memberSize = 1; memberAlignment = 1; }
                else                { memberSize = 4; memberAlignment = 4; } // int / signed / unsigned
                memberSigned = !hasUnsigned && !hasBool;
            } else if (typeSpec.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                       && typeSpec.su) {
                // Defer the nested layout: a pointer member keeps pointer size,
                // an embedded member recurses (below, per declarator).
                structBaseSu = typeSpec.su.get();
                memberSize = 8;       // pointer-member default
                memberAlignment = 8;
            } else {
                memberSize = 8;
                memberAlignment = 8;
            }
        } else {
            memberSize = 8;
            memberAlignment = 8;
        }

        // Place an ordinary (non-bit-field) field: close any open bit unit,
        // align to a byte boundary, advance.
        auto placeField = [&](const std::string& name, size_t sz, size_t al) {
            size_t fieldOffset;
            if (isUnion) {
                fieldOffset = 0;
                maxMemberSize = std::max(maxMemberSize, sz);
            } else {
                size_t curByte = (offsetBits + 7) / 8;
                curByte = (curByte + al - 1) & ~(al - 1);
                fieldOffset = curByte;
                offsetBits = (curByte + sz) * 8;
            }
            layout.fieldOffsets.emplace_back(name, fieldOffset);
            layout.byteAlignment = std::max(layout.byteAlignment, al);
        };

        // Place a bit-field of `width` bits, storage unit `sz` bytes, LSB-first.
        auto placeBitField = [&](const std::string& name, size_t sz, size_t al,
                                 unsigned width, bool isSigned) {
            layout.byteAlignment = std::max(layout.byteAlignment, al);
            size_t tBits = sz * 8;
            if (width == 0) {
                // Zero-width bit-field: round up to the next storage-unit
                // boundary (no named field placed).
                if (!isUnion && tBits) offsetBits = (offsetBits + tBits - 1) / tBits * tBits;
                return;
            }
            size_t startBit = isUnion ? 0 : offsetBits;
            // A field that would straddle a storage-unit boundary moves to the
            // next unit.
            if (tBits && (startBit % tBits) + width > tBits)
                startBit = (startBit + tBits - 1) / tBits * tBits;
            size_t unitIndex = tBits ? startBit / tBits : 0;
            BitFieldInfo bi;
            bi.isBitfield = true;
            bi.bitOffset = (unsigned)(startBit - unitIndex * tBits);
            bi.bitWidth = width;
            bi.storageSize = sz;
            bi.isSigned = isSigned;
            size_t byteOffset = unitIndex * sz;
            layout.fieldOffsets.emplace_back(name, byteOffset);
            layout.bitFields.emplace_back(name, bi);
            if (isUnion) maxMemberSize = std::max(maxMemberSize, sz);
            else offsetBits = startBit + width;
        };

        // A single member declaration may declare several fields
        // (`int a, b, c;`, or mixed `int x, *p, arr[4];`). Each declarator
        // shares the member's base type but carries its own pointer/array
        // adornment and gets its own offset.
        if (member.declarators.empty()) {
            // Anonymous member (e.g. an anonymous struct/union) — embedded by
            // value, so use its true size.
            size_t sz = memberSize, al = memberAlignment;
            if (structBaseSu) {
                StructLayout sub = computeLayout(*structBaseSu);
                sz = sub.byteSize ? sub.byteSize : 1;
                al = sub.byteAlignment;
            }
            placeField("", sz, al);
        } else {
            for (const auto& sd : member.declarators) {
                if (sd.bitfieldWidth.has_value() && *sd.bitfieldWidth) {
                    long long w = 0;
                    const auto& we = **sd.bitfieldWidth;
                    if (we.kind == wvmcc::parser::Expr::Kind::Integer)
                        w = static_cast<const wvmcc::parser::IntegerLiteral&>(we).value;
                    placeBitField(sd.declarator ? declaratorName(sd.declarator) : "",
                                  memberSize, memberAlignment,
                                  (unsigned)(w < 0 ? 0 : w), memberSigned);
                } else {
                    size_t sz = memberSize, al = memberAlignment;
                    // A struct/union member with no pointer adornment is embedded
                    // by value: recurse for its true size/alignment (a pointer
                    // member keeps the 8-byte default — and never recurses, so
                    // self-referential `struct node *next` terminates).
                    bool isPointer = false;
                    if (sd.declarator)
                        for (auto cur = sd.declarator; cur;
                             cur = (cur->inner.has_value() ? *cur->inner : nullptr))
                            if (cur->kind == wvmcc::parser::Declarator::Kind::Pointer) { isPointer = true; break; }
                    if (structBaseSu && !isPointer) {
                        StructLayout sub = computeLayout(*structBaseSu);
                        sz = sub.byteSize ? sub.byteSize : 1;
                        al = sub.byteAlignment;
                        if (sd.declarator) applyDeclaratorToLayout(sd.declarator, sz, al); // array multiplier
                    } else if (sd.declarator) {
                        applyDeclaratorToLayout(sd.declarator, sz, al);
                    }
                    placeField(sd.declarator ? declaratorName(sd.declarator) : "", sz, al);
                }
            }
        }
    }

    // Final size: structs pad to alignment; unions are max-member-size padded.
    size_t finalAlignment = layout.byteAlignment;
    size_t rawSize = isUnion ? maxMemberSize : (offsetBits + 7) / 8;
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