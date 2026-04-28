#include "ModuleCodegen.hpp"
#include "FunctionCodegen.hpp"
#include <stdexcept>

namespace wvmcc::codegen {

ModuleCodegen::ModuleCodegen(const wvmcc::parser::Semantic& semantic)
    : semantic_(semantic) {}

WasmVM::WasmModule ModuleCodegen::generate(const wvmcc::parser::TranslationUnitPtr& tu) {
    // Initialize the module
    module_ = WasmVM::WasmModule{};
    
    // Setup memory types as specified in the lowering plan
    setupMemory();
    
    // Setup global variables
    setupGlobals();
    
    // First pass: register symbols and imports
    firstPass(tu);
    
    // Second pass: generate function bodies
    secondPass(tu);
    
    return module_;
}

void ModuleCodegen::setupMemory() {
    // Emit two MemType entries as specified in the lowering plan:
    // mem[0] (heap/static, 1 page min, is64=true)
    // mem[1] (shadow stack, 1 page min, is64=true)
    // Indices 2–15 reserved
    
    WasmVM::MemType mem0;
    mem0.min = 1;  // 1 page min
    mem0.is64 = true;
    mem0.max = std::nullopt;  // No max limit
    
    WasmVM::MemType mem1;
    mem1.min = 1;  // 1 page min
    mem1.is64 = true;
    mem1.max = std::nullopt;  // No max limit
    
    module_.mems.push_back(mem0);
    module_.mems.push_back(mem1);
    
    // Reserve indices 2-15 for future use
    for (int i = 2; i <= 15; ++i) {
        // These will be reserved but not used yet
    }
}

void ModuleCodegen::setupGlobals() {
    // Emit __stack_pointer mutable i64 global (init = top of mem[1], e.g. 0x10000)
    WasmVM::WasmGlobal stackPointerGlobal;
    stackPointerGlobal.type = WasmVM::GlobalType{WasmVM::GlobalType::variable, WasmVM::ValueType::i64};
    stackPointerGlobal.init = WasmVM::Instr::I64_const{0x10000};  // Top of mem[1]
    
    module_.globals.push_back(stackPointerGlobal);
}

void ModuleCodegen::firstPass(const wvmcc::parser::TranslationUnitPtr& tu) {
    if (!tu) return;
    
    // Iterate TU externals; register every function definition and extern function declaration
    for (const auto& external : tu->externals) {
        emitExternalDecl(external);
    }
}

void ModuleCodegen::secondPass(const wvmcc::parser::TranslationUnitPtr& tu) {
    if (!tu) return;
    
    // Second pass: generate function bodies
    for (const auto& external : tu->externals) {
        emitExternalDecl(external);
    }
}

void ModuleCodegen::emitExternalDecl(const wvmcc::parser::ExternalDeclPtr& decl) {
    if (!decl) return;
    
    std::visit([this](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, wvmcc::parser::FunctionDefPtr>) {
            emitFunctionDefinition(item);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::DeclarationPtr>) {
            emitDeclaration(item);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::ExternalDecl::StaticAssertPtr>) {
            // Static assertions are not code-generated, they're evaluated at semantic phase
        }
    }, decl->decl);
}

void ModuleCodegen::emitDeclaration(const wvmcc::parser::DeclarationPtr& decl) {
    if (!decl) return;
    
    // Check if this is a function declaration
    if (decl->declarator && decl->declarator->kind == wvmcc::parser::Declarator::Kind::Function) {
        emitFunctionDeclaration(decl);
    } else {
        // Handle variable declarations
        if (decl->specifiers.hasStorage(wvmcc::parser::StorageClass::Static)) {
            // Static variables are handled in second pass
        } else {
            // Regular variables (local or global)
            emitGlobalScalar(decl);
        }
    }
}

void ModuleCodegen::emitFunctionDeclaration(const wvmcc::parser::DeclarationPtr& decl) {
    // For now, just register the function in the symbol table
    // In a real implementation, this would handle function imports
}

void ModuleCodegen::emitFunctionDefinition(const wvmcc::parser::FunctionDefPtr& funcDef) {
    // Create a function code generator for this function
    FunctionCodegen funcCodegen(typeMap_, symbolTable_);
    
    // Generate the function
    auto func = funcCodegen.generate(funcDef, semantic_);
    
    // Add to module
    module_.funcs.push_back(func);
    
    // Register function in symbol table
    // This is a placeholder - in a real implementation we'd track function indices
}

void ModuleCodegen::emitGlobalScalar(const wvmcc::parser::DeclarationPtr& decl) {
    // Register file-scope scalar variables as WasmGlobal (i64, mutable, init 0)
    // This is a placeholder - in a real implementation we'd handle the actual declaration
    
    // For now, we'll just add a dummy global
    WasmVM::WasmGlobal global;
    global.type = WasmVM::GlobalType{WasmVM::GlobalType::variable, WasmVM::ValueType::i64};
    global.init = WasmVM::Instr::I64_const{0};  // init 0
    
    module_.globals.push_back(global);
}

void ModuleCodegen::emitGlobalAggregate(const wvmcc::parser::DeclarationPtr& decl) {
    // Placeholder for handling global aggregates
    // In a real implementation, this would allocate space in the data segment
}

void ModuleCodegen::emitStringLiterals() {
    // Placeholder for emitting string literals as data segments
    // In a real implementation, this would collect all string literals from the translation unit
    // and emit them as data segments in the Wasm module
    
    // For now, this is a placeholder - in a real implementation:
    // 1. Collect all string literals from the translation unit
    // 2. Use dataAllocator_ to allocate space for each string literal
    // 3. Create Wasm data segments with the string content
    
    // This will be implemented in later phases as we build out the full implementation
}

} // namespace wvmcc::codegen