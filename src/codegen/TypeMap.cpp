#include "TypeMap.hpp"
#include "LayoutEngine.hpp"
#include "../parser/Semantic.hpp"
#include "../parser/ConstExprEval.hpp"

namespace wvmcc::codegen {

WasmVM::ValueType TypeMap::toWasmType(const wvmcc::parser::TypeNodePtr& type) const {
    if (!type) {
        return WasmVM::ValueType::i32; // Default fallback
    }
    
    switch (type->kind) {
        case wvmcc::parser::TypeNode::Kind::Builtin: {
            // Handle builtin types
            if (type->simple.empty()) {
                return WasmVM::ValueType::i32; // Default fallback
            }
            // Scan all specifiers — a multi-token declaration like
            // `unsigned long` has simple=[Unsigned, Long] and resolves to i64.
            using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
            bool hasDouble = false, hasFloat = false, hasLong = false;
            for (auto s : type->simple) {
                if (s == STS::Double) hasDouble = true;
                else if (s == STS::Float) hasFloat = true;
                else if (s == STS::Long)  hasLong  = true;
            }
            if (hasDouble) return WasmVM::ValueType::f64; // long double → f64
            if (hasFloat)  return WasmVM::ValueType::f32;
            if (hasLong)   return WasmVM::ValueType::i64;
            if (type->simple[0] == STS::Void) return WasmVM::ValueType::i32;
            return WasmVM::ValueType::i32;
        }
        case wvmcc::parser::TypeNode::Kind::Pointer: {
            // M2-L7: pointer-to-function uses Wasm's funcref reference type
            // (via ref.func / call_ref) so the linker can renumber function
            // indices when merging without needing to rewrite hardcoded slot
            // numbers in user code. Other pointers stay i64 (wasm64).
            if (type->pointee &&
                type->pointee->kind == wvmcc::parser::TypeNode::Kind::Function) {
                return WasmVM::ValueType::funcref;
            }
            return WasmVM::ValueType::i64;
        }
        case wvmcc::parser::TypeNode::Kind::Array: {
            // Arrays are memory resident, represented by i64 address
            return WasmVM::ValueType::i64;
        }
        case wvmcc::parser::TypeNode::Kind::Function: {
            // M2-L7: bare function "type" decays to funcref in value context
            // (matches the pointer-to-function case above).
            return WasmVM::ValueType::funcref;
        }
        case wvmcc::parser::TypeNode::Kind::Struct:
        case wvmcc::parser::TypeNode::Kind::Union: {
            // Structs and unions are memory resident, represented by i64 address
            return WasmVM::ValueType::i64;
        }
        case wvmcc::parser::TypeNode::Kind::Enum: {
            // Enums are represented as i32
            return WasmVM::ValueType::i32;
        }
        case wvmcc::parser::TypeNode::Kind::Qualified: {
            // Qualified types inherit the base type
            if (type->pointee) {
                return toWasmType(type->pointee);
            }
            return WasmVM::ValueType::i32;
        }
        default:
            return WasmVM::ValueType::i32;
    }
}

size_t TypeMap::byteSize(const wvmcc::parser::TypeNodePtr& type) const {
    if (!type) {
        return 0;
    }
    
    switch (type->kind) {
        case wvmcc::parser::TypeNode::Kind::Builtin: {
            if (type->simple.empty()) {
                return 0;
            }
            using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
            bool hasDouble = false, hasFloat = false, hasLong = false, hasShort = false;
            for (auto s : type->simple) {
                if (s == STS::Double) hasDouble = true;
                else if (s == STS::Float) hasFloat = true;
                else if (s == STS::Long)  hasLong  = true;
                else if (s == STS::Short) hasShort = true;
            }
            if (hasDouble) return 8; // long double → double in wvmcc
            if (hasFloat)  return 4;
            if (hasLong)   return 8;
            if (hasShort)  return 2;
            auto simpleType = type->simple[0];
            switch (simpleType) {
                case STS::Void: return 0;
                case STS::Bool:
                case STS::Char: return 1;
                default:        return 4;
            }
        }
        case wvmcc::parser::TypeNode::Kind::Pointer: {
            // Pointers are 8 bytes in Wasm64
            return 8;
        }
        case wvmcc::parser::TypeNode::Kind::Array: {
            if (!type->element) return 0;
            size_t elemSize = byteSize(type->element);
            if (type->sizeExpr) {
                auto v = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(*type->sizeExpr);
                if (v.has_value() && *v > 0) {
                    return elemSize * (size_t)*v;
                }
            }
            return elemSize;
        }
        case wvmcc::parser::TypeNode::Kind::Struct:
        case wvmcc::parser::TypeNode::Kind::Union: {
            // For structs/unions, use LayoutEngine to calculate size and alignment
            if (type->su) {
                auto layout = layoutEngine_.computeLayout(*type->su);
                return layout.byteSize;
            }
            return 8; // Fallback
        }
        case wvmcc::parser::TypeNode::Kind::Enum: {
            // Enums are typically represented as int-sized values
            return 4;
        }
        case wvmcc::parser::TypeNode::Kind::Qualified: {
            if (type->pointee) {
                return byteSize(type->pointee);
            }
            return 4;
        }
        default:
            return 4;
    }
}

size_t TypeMap::byteAlignment(const wvmcc::parser::TypeNodePtr& type) const {
    if (!type) {
        return 1;
    }
    
    switch (type->kind) {
        case wvmcc::parser::TypeNode::Kind::Builtin: {
            if (type->simple.empty()) {
                return 1;
            }
            // Alignment follows size for these scalar types.
            size_t sz = byteSize(type);
            return sz > 0 ? sz : 1;
        }
        case wvmcc::parser::TypeNode::Kind::Pointer: {
            // Pointers are 8 bytes aligned in Wasm64
            return 8;
        }
        case wvmcc::parser::TypeNode::Kind::Array: {
            // Arrays align to their element type
            if (type->element) {
                return byteAlignment(type->element);
            }
            return 1;
        }
        case wvmcc::parser::TypeNode::Kind::Struct:
        case wvmcc::parser::TypeNode::Kind::Union: {
            // For structs/unions, use LayoutEngine to calculate alignment
            if (type->su) {
                auto layout = layoutEngine_.computeLayout(*type->su);
                return layout.byteAlignment;
            }
            return 8; // Fallback
        }
        case wvmcc::parser::TypeNode::Kind::Enum: {
            // Enums align like integers
            return 4;
        }
        case wvmcc::parser::TypeNode::Kind::Qualified: {
            if (type->pointee) {
                return byteAlignment(type->pointee);
            }
            return 4;
        }
        default:
            return 4;
    }
}

bool TypeMap::isMemoryResident(const wvmcc::parser::TypeNodePtr& type) const {
    if (!type) {
        return false;
    }
    
    switch (type->kind) {
        case wvmcc::parser::TypeNode::Kind::Builtin:
            // Basic types are not memory resident
            return false;
        case wvmcc::parser::TypeNode::Kind::Pointer:
            // Pointers are memory resident (they point to memory)
            return true;
        case wvmcc::parser::TypeNode::Kind::Array:
            // Arrays are memory resident
            return true;
        case wvmcc::parser::TypeNode::Kind::Function:
            // Functions are not memory resident
            return false;
        case wvmcc::parser::TypeNode::Kind::Struct:
            // Structs are memory resident
            return true;
        case wvmcc::parser::TypeNode::Kind::Union:
            // Unions are memory resident
            return true;
        case wvmcc::parser::TypeNode::Kind::Enum:
            // Enums are not memory resident
            return false;
        case wvmcc::parser::TypeNode::Kind::Qualified:
            // Qualified types inherit from their pointee
            if (type->pointee) {
                return isMemoryResident(type->pointee);
            }
            return false;
        default:
            return false;
    }
}

WasmVM::WasmInstr TypeMap::makeLoad(const wvmcc::parser::TypeNodePtr& type, uint8_t memidx) const {
    auto wasmType = toWasmType(type);

    // Alignment field is log2 of byte alignment per Wasm spec.
    switch (wasmType) {
        case WasmVM::ValueType::i32:
            return WasmVM::Instr::I32_load{memidx, 0, 2}; // log2(4) = 2
        case WasmVM::ValueType::i64:
            return WasmVM::Instr::I64_load{memidx, 0, 3}; // log2(8) = 3
        case WasmVM::ValueType::f32:
            return WasmVM::Instr::F32_load{memidx, 0, 2};
        case WasmVM::ValueType::f64:
            return WasmVM::Instr::F64_load{memidx, 0, 3};
        default:
            return WasmVM::Instr::I32_load{memidx, 0, 2};
    }
}

WasmVM::WasmInstr TypeMap::makeStore(const wvmcc::parser::TypeNodePtr& type, uint8_t memidx) const {
    auto wasmType = toWasmType(type);

    switch (wasmType) {
        case WasmVM::ValueType::i32:
            return WasmVM::Instr::I32_store{memidx, 0, 2};
        case WasmVM::ValueType::i64:
            return WasmVM::Instr::I64_store{memidx, 0, 3};
        case WasmVM::ValueType::f32:
            return WasmVM::Instr::F32_store{memidx, 0, 2};
        case WasmVM::ValueType::f64:
            return WasmVM::Instr::F64_store{memidx, 0, 3};
        default:
            return WasmVM::Instr::I32_store{memidx, 0, 2};
    }
}

size_t TypeMap::getFieldOffset(const wvmcc::parser::TypeNodePtr& type, const std::string& fieldName) const {
    if (!type || !type->su) return 0;
    auto layout = layoutEngine_.computeLayout(*type->su);
    for (const auto& [name, offset] : layout.fieldOffsets) {
        if (name == fieldName) return offset;
    }
    return 0;
}

// Walk a declarator chain to its Identifier node, returning the declared
// name (empty if none). The parser builds the chain outer→inner as
//   [trailing-suffix(Array/Function)] → Identifier → [leading `*`s]
static std::string declaratorName(const wvmcc::parser::DeclaratorPtr& decl) {
    for (auto cur = decl; cur; cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (cur->kind == wvmcc::parser::Declarator::Kind::Identifier
            || (!cur->id.name.empty()))
            return cur->id.name;
    }
    return std::string();
}

// Apply the pointer / array layers of a declarator chain on top of a base
// type, mirroring ModuleCodegen::buildReturnTypeNode. Layers that decorate
// the member sit both above (trailing array suffixes) and below (leading
// `*`s) the Identifier; collect them all and wrap from innermost outward.
static wvmcc::parser::TypeNodePtr applyDeclaratorLayers(
    wvmcc::parser::TypeNodePtr baseType,
    const wvmcc::parser::DeclaratorPtr& decl) {
    std::vector<const wvmcc::parser::Declarator*> belowId; // leading `*`s / arrays under id
    std::vector<const wvmcc::parser::Declarator*> aboveId; // trailing suffixes over id
    bool sawId = false;
    for (auto cur = decl; cur; cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (cur->kind == wvmcc::parser::Declarator::Kind::Identifier) { sawId = true; continue; }
        if (cur->kind == wvmcc::parser::Declarator::Kind::Pointer
            || cur->kind == wvmcc::parser::Declarator::Kind::Array) {
            if (sawId) belowId.push_back(cur.get());
            else       aboveId.push_back(cur.get());
        }
    }
    auto wrap = [&](const wvmcc::parser::Declarator* d) {
        if (d->kind == wvmcc::parser::Declarator::Kind::Pointer) {
            auto n = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            n->kind = wvmcc::parser::TypeNode::Kind::Pointer;
            n->pointee = baseType;
            baseType = n;
        } else { // Array
            auto n = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            n->kind = wvmcc::parser::TypeNode::Kind::Array;
            n->element = baseType;
            baseType = n;
        }
    };
    // innermost first: the `*`s below the identifier, then trailing suffixes.
    for (auto it = belowId.rbegin(); it != belowId.rend(); ++it) wrap(*it);
    for (auto it = aboveId.begin(); it != aboveId.end(); ++it) wrap(*it);
    return baseType;
}

wvmcc::parser::TypeNodePtr TypeMap::getFieldType(const wvmcc::parser::TypeNodePtr& type, const std::string& fieldName) const {
    if (!type || !type->su) return nullptr;
    for (const auto& member : type->su->members) {
        for (const auto& sd : member.declarators) {
            if (!sd.declarator) continue;
            if (declaratorName(sd.declarator) != fieldName) continue;
            // Prefer the semantic resolver: it follows typedef-name chains
            // (e.g. `FILE *`, `size_t`) and applies all declarator layers,
            // which the manual reconstruction below cannot.
            if (semantic_) {
                if (auto resolved =
                        semantic_->canonicalTypeRepr(member.specifiers, sd.declarator))
                    return resolved;
            }
            wvmcc::parser::TypeNodePtr baseType = nullptr;
            for (const auto& ts : member.specifiers.typeSpecifiers) {
                if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                    && !ts.simple.empty()) {
                    baseType = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    baseType->kind = wvmcc::parser::TypeNode::Kind::Builtin;
                    baseType->simple = ts.simple;
                    break;
                }
                if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                    && ts.su) {
                    baseType = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    baseType->kind = (ts.su->kind == wvmcc::parser::StructOrUnionSpecifier::Kind::Struct)
                                 ? wvmcc::parser::TypeNode::Kind::Struct
                                 : wvmcc::parser::TypeNode::Kind::Union;
                    baseType->su = ts.su;
                    break;
                }
            }
            if (!baseType) return nullptr;
            return applyDeclaratorLayers(baseType, sd.declarator);
        }
    }
    return nullptr;
}

std::vector<std::string> TypeMap::getOrderedFieldNames(const wvmcc::parser::TypeNodePtr& type) const {
    std::vector<std::string> names;
    if (!type || !type->su) return names;
    for (const auto& member : type->su->members) {
        for (const auto& sd : member.declarators) {
            if (!sd.declarator) continue;
            auto n = declaratorName(sd.declarator);
            if (!n.empty()) names.push_back(n);
        }
    }
    return names;
}

WasmVM::ValueType TypeMap::getBaseType(const wvmcc::parser::TypeNodePtr& type) const {
    if (!type) {
        return WasmVM::ValueType::i32;
    }
    
    switch (type->kind) {
        case wvmcc::parser::TypeNode::Kind::Builtin:
            return toWasmType(type);
        case wvmcc::parser::TypeNode::Kind::Pointer:
            if (type->pointee) {
                return getBaseType(type->pointee);
            }
            return WasmVM::ValueType::i64;
        case wvmcc::parser::TypeNode::Kind::Array:
            if (type->element) {
                return getBaseType(type->element);
            }
            return WasmVM::ValueType::i32;
        case wvmcc::parser::TypeNode::Kind::Function:
            return WasmVM::ValueType::i64;
        case wvmcc::parser::TypeNode::Kind::Struct:
        case wvmcc::parser::TypeNode::Kind::Union:
            return WasmVM::ValueType::i64;
        case wvmcc::parser::TypeNode::Kind::Enum:
            return WasmVM::ValueType::i32;
        case wvmcc::parser::TypeNode::Kind::Qualified:
            if (type->pointee) {
                return getBaseType(type->pointee);
            }
            return WasmVM::ValueType::i32;
        default:
            return WasmVM::ValueType::i32;
    }
}

size_t TypeMap::getSimpleTypeSize(const wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier& simpleType) const {
    switch (simpleType) {
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void:
            return 0;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Bool:
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Char:
            return 1;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Short:
            return 2;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int:
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Signed:
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Unsigned:
            return 4;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Long:
            return 8;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Float:
            return 4;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Double:
            return 8;
        default:
            return 4;
    }
}

size_t TypeMap::getSimpleTypeAlignment(const wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier& simpleType) const {
    switch (simpleType) {
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void:
            return 1;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Bool:
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Char:
            return 1;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Short:
            return 2;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int:
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Signed:
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Unsigned:
            return 4;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Long:
            return 8;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Float:
            return 4;
        case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Double:
            return 8;
        default:
            return 4;
    }
}

} // namespace wvmcc::codegen