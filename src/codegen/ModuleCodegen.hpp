#pragma once

#include "TypeMap.hpp"
#include "SymbolTable.hpp"
#include "TypeIndexCache.hpp"
#include "GlobalDataAllocator.hpp"
#include "FunctionCodegen.hpp"
#include "../parser/AST.hpp"
#include "../parser/Semantic.hpp"
#include <WasmVM.hpp>
#include <vector>
#include <unordered_set>

namespace wvmcc::codegen {

class ModuleCodegen {
public:
    ModuleCodegen(const wvmcc::parser::Semantic& semantic);
    
    // Generate a Wasm module from a translation unit
    WasmVM::WasmModule generate(const wvmcc::parser::TranslationUnitPtr& tu);
    
private:
    const wvmcc::parser::Semantic& semantic_;
    
    // Code generation components
    TypeMap typeMap_;
    SymbolTable symbolTable_;
    TypeIndexCache typeIndexCache_;
    GlobalDataAllocator dataAllocator_;
    
    // Module being built
    WasmVM::WasmModule module_;
    
    // Function code generators
    std::vector<FunctionCodegen> functionCodegens_;
    
    // Helper functions
    void setupMemory();
    void setupGlobals();
    void firstPass(const wvmcc::parser::TranslationUnitPtr& tu);
    void secondPass(const wvmcc::parser::TranslationUnitPtr& tu);
    void emitFunction(const wvmcc::parser::FunctionDefPtr& funcDef);
    void emitExternalDecl(const wvmcc::parser::ExternalDeclPtr& decl);
    void emitDeclaration(const wvmcc::parser::DeclarationPtr& decl);
    void emitFunctionDeclaration(const wvmcc::parser::DeclarationPtr& decl);
    void emitFunctionDefinition(const wvmcc::parser::FunctionDefPtr& funcDef);
    void emitGlobalScalar(const wvmcc::parser::DeclarationPtr& decl);
    void emitGlobalAggregate(const wvmcc::parser::DeclarationPtr& decl);
    void emitStringLiterals();
};

} // namespace wvmcc::codegen