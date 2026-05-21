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
#include <unordered_map>
#include <unordered_set>

namespace wvmcc::codegen {

// Compile mode controls how each TU is emitted. Freestanding produces a
// self-contained module (M1-style: own memories, own stack pointer, start
// wrapper around main calling sys_proc.exit). Linkable (M2-D) produces an
// object suitable for the integrated linker — imports `env.__*` runtime
// state and emits no start wrapper.
enum class CompileMode {
    Freestanding,
    Linkable,
};

class ModuleCodegen {
public:
    ModuleCodegen(const wvmcc::parser::Semantic& semantic);

    // Set the compile mode. Default is Freestanding (M2-F-era; M2-D flips
    // this to Linkable by default).
    void setCompileMode(CompileMode mode) { compileMode_ = mode; }
    CompileMode getCompileMode() const { return compileMode_; }

    // Generate a Wasm module from a translation unit
    WasmVM::WasmModule generate(const wvmcc::parser::TranslationUnitPtr& tu);

    // Public hooks used by FunctionCodegen during body emission.
    // Returns table-slot index for an address-taken function, or std::nullopt.
    std::optional<size_t> getFuncTableSlot(const std::string& name) const;
    // typeIdx in module_.types for a function by name (looked up via FuncSymbol).
    std::optional<WasmVM::index_t> getFuncTypeIdx(const std::string& name) const;
    // Append a new mutable i32 global initialized to 0; return its index.
    WasmVM::index_t allocateGuardGlobal();
    // Allocate space in mem[0] for a static local; return its address.
    size_t allocateStaticStorage(size_t size, size_t align);
    // Intern a function type into module_.types (deduplicates).
    WasmVM::index_t internFuncType(const WasmVM::FuncType& ft);

private:
    const wvmcc::parser::Semantic& semantic_;
    CompileMode compileMode_ = CompileMode::Freestanding;

    // Code generation components
    TypeMap typeMap_;
    SymbolTable symbolTable_;
    TypeIndexCache typeIndexCache_;
    GlobalDataAllocator dataAllocator_;

    // Module being built
    WasmVM::WasmModule module_;

    // Monotonically increasing function index counter (imports + defs share one space)
    int nextFuncIndex_ = 0;

    // Function-name → type-idx (in module_.types) for every registered function.
    // Filled during firstPass() so indirect calls and `&func` know the FuncType.
    std::unordered_map<std::string, WasmVM::index_t> funcTypeIdx_;

    // Function-name → table-slot index (in funcref table 0) for every
    // address-taken function. Populated by analyzeFuncAddressTaken().
    std::unordered_map<std::string, size_t> funcTableSlots_;

    // Hosted-environment state (issue #40). When the translation unit defines
    // `main`, ModuleCodegen pre-injects four sys_proc imports and emits a
    // hidden start-section wrapper that calls `main` with argc/argv before
    // forwarding the result to sys_proc.exit.
    bool hasMain_ = false;
    bool mainHasArgv_ = false;
    int mainFuncIndex_ = -1;
    int sysProcArgcIdx_ = -1;
    int sysProcArgvLenIdx_ = -1;
    int sysProcArgvIdx_ = -1;
    int sysProcExitIdx_ = -1;
    void scanForMain(const wvmcc::parser::TranslationUnitPtr& tu);
    void injectSysProcImports();
    void emitStartWrapper();

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
    // Walk every function body to collect &funcname expressions; allocate
    // table slots and emit a funcref table + element segment.
    void analyzeFuncAddressTaken(const wvmcc::parser::TranslationUnitPtr& tu);
    void secondPass(const wvmcc::parser::TranslationUnitPtr& tu);
    void emitFunctionDefinition(const wvmcc::parser::FunctionDefPtr& funcDef);
    void emitGlobalScalar(const wvmcc::parser::DeclarationPtr& decl);
    void emitGlobalAggregate(const wvmcc::parser::DeclarationPtr& decl);
    void emitStringLiterals();
};

} // namespace wvmcc::codegen