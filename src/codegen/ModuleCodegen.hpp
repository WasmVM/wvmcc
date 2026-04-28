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

    // Monotonically increasing function index counter (imports + defs share one space)
    int nextFuncIndex_ = 0;

    // Intern a FuncType into module_.types (deduplicates)
    WasmVM::index_t internFuncType(const WasmVM::FuncType& ft);

    // Extract the identifier name from a (possibly nested) declarator
    std::string getFuncName(const wvmcc::parser::DeclaratorPtr& decl) const;

    // Build WasmVM::FuncType from a function definition or extern declaration
    WasmVM::FuncType buildFuncTypeFromDef(const wvmcc::parser::FunctionDefPtr& funcDef) const;
    WasmVM::FuncType buildFuncTypeFromDecl(const wvmcc::parser::DeclarationPtr& decl) const;

    // First-pass: register types and symbols without generating bodies
    void registerExternalDecl(const wvmcc::parser::ExternalDeclPtr& decl);
    void registerFunctionDef(const wvmcc::parser::FunctionDefPtr& funcDef);
    void registerFunctionDeclaration(const wvmcc::parser::DeclarationPtr& decl);

    void setupMemory();
    void setupGlobals();
    void firstPass(const wvmcc::parser::TranslationUnitPtr& tu);
    void secondPass(const wvmcc::parser::TranslationUnitPtr& tu);
    void emitFunctionDefinition(const wvmcc::parser::FunctionDefPtr& funcDef);
    void emitGlobalScalar(const wvmcc::parser::DeclarationPtr& decl);
    void emitGlobalAggregate(const wvmcc::parser::DeclarationPtr& decl);
    void emitStringLiterals();
};

} // namespace wvmcc::codegen