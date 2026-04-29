#include "TypeMap.hpp"
#include "LayoutEngine.hpp"
#include "../parser/Semantic.hpp"

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
            
            // Get the first simple type specifier
            auto simpleType = type->simple[0];
            switch (simpleType) {
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void:
                    return WasmVM::ValueType::i32; // void: no WasmVM None; callers must handle empty result list separately
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Bool:
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Char:
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Short:
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Int:
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Signed:
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Unsigned:
                    return WasmVM::ValueType::i32;
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Long:
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Complex:
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Imaginary:
                    return WasmVM::ValueType::i64;
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Float:
                    return WasmVM::ValueType::f32;
                case wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Double:
                    return WasmVM::ValueType::f64;
                default:
                    return WasmVM::ValueType::i32;
            }
            break;
        }
        case wvmcc::parser::TypeNode::Kind::Pointer: {
            // Pointers are i64 in Wasm64
            return WasmVM::ValueType::i64;
        }
        case wvmcc::parser::TypeNode::Kind::Array: {
            // Arrays are memory resident, represented by i64 address
            return WasmVM::ValueType::i64;
        }
        case wvmcc::parser::TypeNode::Kind::Function: {
            // Functions are represented by i64 addresses in Wasm64
            return WasmVM::ValueType::i64;
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
            
            auto simpleType = type->simple[0];
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
            break;
        }
        case wvmcc::parser::TypeNode::Kind::Pointer: {
            // Pointers are 8 bytes in Wasm64
            return 8;
        }
        case wvmcc::parser::TypeNode::Kind::Array: {
            // For arrays, we need to calculate based on element size and count
            if (type->element && type->sizeExpr) {
                // This is a simplified approach - in a real implementation we'd evaluate the size expression
                return byteSize(type->element) * 1; // Placeholder
            }
            return 0;
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
            
            auto simpleType = type->simple[0];
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
            break;
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
    
    switch (wasmType) {
        case WasmVM::ValueType::i32:
            return WasmVM::Instr::I32_load{memidx, 0, 4};
        case WasmVM::ValueType::i64:
            return WasmVM::Instr::I64_load{memidx, 0, 8};
        case WasmVM::ValueType::f32:
            return WasmVM::Instr::F32_load{memidx, 0, 4};
        case WasmVM::ValueType::f64:
            return WasmVM::Instr::F64_load{memidx, 0, 8};
        default:
            return WasmVM::Instr::I32_load{memidx, 0, 4};
    }
}

WasmVM::WasmInstr TypeMap::makeStore(const wvmcc::parser::TypeNodePtr& type, uint8_t memidx) const {
    auto wasmType = toWasmType(type);
    
    switch (wasmType) {
        case WasmVM::ValueType::i32:
            return WasmVM::Instr::I32_store{memidx, 0, 4};
        case WasmVM::ValueType::i64:
            return WasmVM::Instr::I64_store{memidx, 0, 8};
        case WasmVM::ValueType::f32:
            return WasmVM::Instr::F32_store{memidx, 0, 4};
        case WasmVM::ValueType::f64:
            return WasmVM::Instr::F64_store{memidx, 0, 8};
        default:
            return WasmVM::Instr::I32_store{memidx, 0, 4};
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

wvmcc::parser::TypeNodePtr TypeMap::getFieldType(const wvmcc::parser::TypeNodePtr& type, const std::string& fieldName) const {
    if (!type || !type->su) return nullptr;
    for (const auto& member : type->su->members) {
        for (const auto& sd : member.declarators) {
            if (!sd.declarator) continue;
            std::string name = sd.declarator->id.name;
            if (name != fieldName) continue;
            for (const auto& ts : member.specifiers.typeSpecifiers) {
                if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
                    && !ts.simple.empty()) {
                    auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    node->kind = wvmcc::parser::TypeNode::Kind::Builtin;
                    node->simple = ts.simple;
                    return node;
                }
                if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::StructOrUnion
                    && ts.su) {
                    auto node = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
                    node->kind = (ts.su->kind == wvmcc::parser::StructOrUnionSpecifier::Kind::Struct)
                                 ? wvmcc::parser::TypeNode::Kind::Struct
                                 : wvmcc::parser::TypeNode::Kind::Union;
                    node->su = ts.su;
                    return node;
                }
            }
        }
    }
    return nullptr;
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