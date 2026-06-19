#pragma once

#include <cstdint>
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
#include <optional>
#include <utility>

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

    // Supply the parser's resolved enum-constant values so FunctionCodegen can
    // resolve an enum constant used in a runtime expression (the parser folds
    // them only at file scope / in constant expressions, leaving in-body uses
    // for scope-aware resolution here — after symbol lookup fails, a local can
    // no longer be shadowing the name). See Parser::getEnumConstants.
    void setEnumConstants(const std::unordered_map<std::string, long long>& m) { enumConstants_ = m; }

    // Generate a Wasm module from a translation unit
    WasmVM::WasmModule generate(const wvmcc::parser::TranslationUnitPtr& tu);

    // Public hooks used by FunctionCodegen during body emission.
    // Resolve an enumeration constant by name to its int value, or nullopt.
    std::optional<long long> lookupEnumConstant(const std::string& name) const {
        auto it = enumConstants_.find(name);
        if (it == enumConstants_.end()) return std::nullopt;
        return it->second;
    }
    // Returns table-slot index for an address-taken function, or std::nullopt.
    std::optional<size_t> getFuncTableSlot(const std::string& name) const;
    // #79: assign (or fetch) the funcref-table slot for `name`, called lazily
    // when a function-pointer value is emitted during body generation. After
    // secondPass, emitFuncTable() materializes a funcref table + element
    // segment covering every interned slot.
    size_t internFuncTableSlot(const std::string& name);
    // typeIdx in module_.types for a function by name (looked up via FuncSymbol).
    std::optional<WasmVM::index_t> getFuncTypeIdx(const std::string& name) const;
    // Append a new mutable i32 global initialized to 0; return its index.
    WasmVM::index_t allocateGuardGlobal();
    // Allocate space in mem[0] for a static local; return its address.
    size_t allocateStaticStorage(size_t size, size_t align);
    // Intern a function type into module_.types (deduplicates).
    WasmVM::index_t internFuncType(const WasmVM::FuncType& ft);

    // Highest linear-memory index in use by this module (>= 1: mem[0] heap +
    // mem[1] shadow stack always exist). Grows when a file-scope object is
    // placed via __attribute__((wvmcc_memidx(N))). FunctionCodegen reads this
    // to size the runtime tag-dispatch in emitTaggedLoad/Store.
    uint8_t maxDataMemidx() const { return maxDataMemidx_; }

    // M2-E: a module-level data-pointer relocation. `codeFuncIdx` is the
    // function's index within `module.funcs` (NOT the module-wide function
    // index space, which would include imports). `instrIdx` is the
    // instruction's position within that function's body. `dataSymbolIdx`
    // indexes `getDataSymbols()`.
    struct Relocation {
        size_t codeFuncIdx;
        size_t instrIdx;
        size_t dataSymbolIdx;
        int64_t addend;
    };
    struct DataSymbol {
        std::string name;
        size_t address; // mem[0] offset of the referenced datum
    };
    const std::vector<Relocation>& getRelocations() const { return relocations_; }
    const std::vector<DataSymbol>& getDataSymbols() const { return dataSymbols_; }

    // #79: a function-pointer relocation site — an `i64.const (tag | slot)`
    // whose embedded funcref-table slot the linker rebases when it merges the
    // per-TU funcref tables. `codeFuncIdx` / `instrIdx` are input-module-local,
    // matching DataPtrSite.
    struct FuncPtrReloc {
        size_t codeFuncIdx;
        size_t instrIdx;
    };
    const std::vector<FuncPtrReloc>& getFuncPtrRelocs() const { return funcPtrRelocs_; }
    // Number of funcref-table slots this TU allocated (table size).
    size_t funcTableSlotCount() const { return funcTableSlots_.size(); }

    // LANG-6.6-06: an address-constant pointer baked into a *data segment*'s
    // bytes (e.g. `static int *p = &obj;`). Unlike code-located i64.const data
    // pointers (Relocation/FuncPtrReloc), these live inside `module.datas`
    // payloads, so the linker rebases them by reading the segment bytes at
    // `byteOffset` rather than rewriting an instruction. `dataIndex` indexes
    // `module.datas`; `byteOffset` is the little-endian i64's offset within
    // that segment's `init`. Data pointers shift by the TU's data-rebase delta;
    // funcptr slots shift by the TU's table-slot delta.
    struct DataSegReloc {
        size_t dataIndex;
        size_t byteOffset;
    };
    const std::vector<DataSegReloc>& getDataSegDataRelocs() const { return dataSegDataRelocs_; }
    const std::vector<DataSegReloc>& getDataSegFuncPtrRelocs() const { return dataSegFuncPtrRelocs_; }

    // Codegen diagnostics accumulated across all functions (errors here mean
    // the emitted module is unsound — the driver should report and not ship it).
    const std::vector<wvmcc::Diagnostic>& getDiagnostics() const { return diagnostics_; }

private:
    std::vector<wvmcc::Diagnostic> diagnostics_;
    const wvmcc::parser::Semantic& semantic_;
    CompileMode compileMode_ = CompileMode::Linkable; // M2-D: default flipped from Freestanding
    // Resolved enum-constant values from the parser (see setEnumConstants).
    std::unordered_map<std::string, long long> enumConstants_{};

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

    // Function-name → table-slot index (in funcref table 0). Interned lazily by
    // internFuncTableSlot() as function-pointer values are emitted, then
    // materialized into a funcref table + element segment by emitFuncTable().
    std::unordered_map<std::string, size_t> funcTableSlots_;

    // M2-E: data symbols (one per distinct string literal so far) and the
    // relocations collected from each FunctionCodegen.
    std::vector<DataSymbol> dataSymbols_;
    std::vector<Relocation> relocations_;
    // #79: function-pointer relocation sites collected across all functions.
    std::vector<FuncPtrReloc> funcPtrRelocs_;
    // Emit the funcref table + active element segment covering funcTableSlots_.
    void emitFuncTable();

    // Cross-TU extern data globals (M2): file-scope object definitions whose
    // address is exported as a Wasm global so other TUs' `extern` references
    // (imported address-globals) resolve to it at link time. Collected during
    // firstPass; materialized by materializeExportedDataGlobals().
    std::vector<std::pair<std::string, size_t>> exportedDataGlobals_;
    // Number of imported Wasm globals (defined globals are indexed after these).
    WasmVM::index_t importedGlobalCount() const;
    // Emit + export the address-globals collected in exportedDataGlobals_.
    void materializeExportedDataGlobals();

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
    // Highest memidx in use (see maxDataMemidx()). 1 unless a global requests
    // an explicit higher placement.
    uint8_t maxDataMemidx_ = 1;
    // Ensure memory `idx` is available for an explicit wvmcc_memidx(N) placement
    // and bump maxDataMemidx_. Freestanding: defines the memory locally (filling
    // gaps). Linkable: records the high-water mark only — the env.__memory_N
    // imports are appended later by materializeMemoryImports(). Always succeeds.
    bool ensureMemory(uint8_t idx);
    // Linkable only: append `env.__memory_N` imports for every placed memory
    // 2..maxDataMemidx_ (crt0 turns them into local memories). Runs after
    // firstPass, once all placements are known.
    void materializeMemoryImports();
    void setupGlobals();
    // Freestanding mode only: patch the `__heap_base` const i64 global (slot
    // heapBaseGlobalIdx_, reserved early in setupGlobals) to
    // round_up_to_8(dataAllocator_.currentTop()). Called after secondPass so
    // the data layout is finalized.
    void finalizeFreestandingHeapBase();
    void firstPass(const wvmcc::parser::TranslationUnitPtr& tu);

    // #77: register the runtime Wasm globals __stack_pointer / __heap_base as
    // GlobalScalar symbols so C references resolve to global.get/global.set,
    // and register every file-scope variable definition (storage in mem[0] +
    // GlobalMem symbol + optional initializer data segment). Runs inside the
    // file scope before function bodies are emitted.
    void registerGlobalVars(const wvmcc::parser::TranslationUnitPtr& tu);
    void registerGlobalVar(const wvmcc::parser::DeclarationPtr& decl);
    // Parse __attribute__((wvmcc_memidx(N))) on a file-scope declaration: returns
    // the validated placement memory N (2..14), or 0 if absent. Emits a
    // diagnostic for an invalid index and calls ensureMemory(N) on success.
    // Used by both the defining and `extern`-referencing paths so an annotated
    // extern (the shared-header idiom) reaches the right memory.
    uint8_t readPlacementMemidx(const wvmcc::parser::DeclarationPtr& decl);
    // Recursively encode a constant initializer (scalar Expr or braced List)
    // for `type` into `out` at byte offset `base` (little-endian). Returns
    // false if any leaf is not a compile-time constant we can encode.
    bool encodeConstInit(const wvmcc::parser::TypeNodePtr& type,
                         const wvmcc::parser::InitializerPtr& init,
                         size_t base, std::vector<std::byte>& out);

    // LANG-6.6-06: an address constant (6.6p9) — a pointer-valued expression
    // whose value is known at link time: `&obj`, array/function decay,
    // `&arr[k]`, `arr + k`, `&s.m`. `value` is the i64 to embed; `isFuncPtr`
    // distinguishes a tagged funcref-table slot (rebased by the table-slot
    // delta) from a mem[0] data address (rebased by the data delta). `pointee`
    // carries the element type so further pointer arithmetic can scale by it.
    struct AddrConst {
        std::int64_t value{0};
        bool isFuncPtr{false};
        wvmcc::parser::TypeNodePtr pointee;
    };
    // A designated object/element (the operand of `&`, or an array/struct
    // sub-object): its mem[0] address and type, or a function designator.
    struct Designator {
        std::int64_t addr{0};
        wvmcc::parser::TypeNodePtr type;
        bool isFunc{false};
        size_t funcSlot{0};
    };
    std::optional<AddrConst> evalAddressConstInit(const wvmcc::parser::ExprPtr& e);
    std::optional<Designator> evalDesignator(const wvmcc::parser::ExprPtr& e);
    // Materialize a file-scope compound literal as a static object: allocate
    // mem[0] storage, encode its initializer into an active data segment, and
    // return the object's address (nullopt if not a constant aggregate). Used by
    // evalAddressConstInit so `static int *p = (int[]){1,2,3};` works.
    std::optional<size_t> materializeCompoundLiteral(const wvmcc::parser::CompoundLiteral& cl);
    // Address-constant sites collected by encodeConstInit for the segment
    // currently being built: (byteOffset within the object's buffer, isFuncPtr).
    // Drained by registerGlobalVar into dataSegDataRelocs_/dataSegFuncPtrRelocs_
    // once the data segment (and thus its module.datas index) exists.
    std::vector<std::pair<size_t, bool>> pendingAddrSites_;
    std::vector<DataSegReloc> dataSegDataRelocs_;
    std::vector<DataSegReloc> dataSegFuncPtrRelocs_;

    // Wasm global indices for the two runtime globals (stable across modes):
    // __stack_pointer = 0, __heap_base = 1. Captured in setupGlobals().
    int stackPtrGlobalIdx_ = -1;
    int heapBaseGlobalIdx_ = -1;
    void secondPass(const wvmcc::parser::TranslationUnitPtr& tu);
    void emitFunctionDefinition(const wvmcc::parser::FunctionDefPtr& funcDef);
    void emitStringLiterals();
};

} // namespace wvmcc::codegen