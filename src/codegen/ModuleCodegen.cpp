#include <cstdint>
#include "ModuleCodegen.hpp"
#include "FunctionCodegen.hpp"
#include "StartWrapper.hpp"
#include "../parser/ConstExprEval.hpp"
#include <stdexcept>
#include <cstring>

namespace wvmcc::codegen {

ModuleCodegen::ModuleCodegen(const wvmcc::parser::Semantic& semantic)
    : semantic_(semantic) {
    // Let TypeMap resolve typedef-name struct-member types (FILE *, size_t, …)
    // via the semantic typedef table.
    typeMap_.setSemantic(&semantic_);
}

WasmVM::WasmModule ModuleCodegen::generate(const wvmcc::parser::TranslationUnitPtr& tu) {
    module_ = WasmVM::WasmModule{};
    nextFuncIndex_ = 0;
    funcTypeIdx_.clear();
    funcTableSlots_.clear();
    funcPtrRelocs_.clear();
    dataSegDataRelocs_.clear();
    dataSegFuncPtrRelocs_.clear();
    pendingAddrSites_.clear();
    hasMain_ = false;
    mainHasArgv_ = false;
    mainFuncIndex_ = -1;
    sysProcArgcIdx_ = sysProcArgvLenIdx_ = sysProcArgvIdx_ = sysProcExitIdx_ = -1;

    setupMemory();
    setupGlobals();

    // Detect main() and (in freestanding mode) inject sys_proc imports before
    // any user functions are registered, so the sys_proc imports occupy
    // function indices 0..3.
    //
    // In linkable mode (M2-D) the start wrapper is synthesized by the
    // linker's crt0 — this TU just emits its own functions and lets the
    // linker discover `main` by name.
    scanForMain(tu);
    const bool emitWrapperHere = hasMain_ && compileMode_ == CompileMode::Freestanding;
    if (emitWrapperHere) {
        injectSysProcImports();
    }

    symbolTable_.pushScope();
    firstPass(tu);
    // All global imports are now known — materialize exported data-address
    // globals so they occupy the first defined-global slots (before any guard
    // globals allocated during secondPass).
    materializeExportedDataGlobals();
    secondPass(tu);
    // #79: now that every function-pointer value has interned its table slot,
    // emit the funcref table + element segment that call_indirect dispatches
    // through (the linker merges per-TU tables when combining objects).
    emitFuncTable();
    symbolTable_.popScope();

    if (compileMode_ == CompileMode::Freestanding) {
        finalizeFreestandingHeapBase();
    }

    if (emitWrapperHere) {
        emitStartWrapper();
    }

    return module_;
}

std::optional<size_t> ModuleCodegen::getFuncTableSlot(const std::string& name) const {
    auto it = funcTableSlots_.find(name);
    if (it == funcTableSlots_.end()) return std::nullopt;
    return it->second;
}

size_t ModuleCodegen::internFuncTableSlot(const std::string& name) {
    auto it = funcTableSlots_.find(name);
    if (it != funcTableSlots_.end()) return it->second;
    size_t slot = funcTableSlots_.size();
    funcTableSlots_.emplace(name, slot);
    return slot;
}

std::optional<WasmVM::index_t> ModuleCodegen::getFuncTypeIdx(const std::string& name) const {
    auto it = funcTypeIdx_.find(name);
    if (it == funcTypeIdx_.end()) return std::nullopt;
    return it->second;
}

WasmVM::index_t ModuleCodegen::importedGlobalCount() const {
    WasmVM::index_t n = 0;
    for (const auto& imp : module_.imports) {
        if (std::holds_alternative<WasmVM::GlobalType>(imp.desc)) ++n;
    }
    return n;
}

void ModuleCodegen::materializeExportedDataGlobals() {
    // Called after firstPass, before any defined global (guard globals) is
    // allocated, so these address-globals are the first defined globals and
    // their indices are stable.
    for (const auto& [name, addr] : exportedDataGlobals_) {
        WasmVM::WasmGlobal g;
        g.type = WasmVM::GlobalType{WasmVM::GlobalType::constant, WasmVM::ValueType::i64};
        g.init = WasmVM::Instr::I64_const{(WasmVM::i64_t)addr};
        module_.globals.push_back(g);
        WasmVM::index_t gidx = importedGlobalCount()
                             + (WasmVM::index_t)(module_.globals.size() - 1);

        WasmVM::WasmExport ex;
        ex.name = name;
        ex.desc = WasmVM::WasmExport::DescType::global;
        ex.index = gidx;
        module_.exports.push_back(ex);
    }
}

WasmVM::index_t ModuleCodegen::allocateGuardGlobal() {
    WasmVM::WasmGlobal g;
    g.type = WasmVM::GlobalType{WasmVM::GlobalType::variable, WasmVM::ValueType::i32};
    g.init = WasmVM::Instr::I32_const{0};
    module_.globals.push_back(g);
    // Defined globals are indexed after all imported globals.
    return importedGlobalCount() + (WasmVM::index_t)(module_.globals.size() - 1);
}

size_t ModuleCodegen::allocateStaticStorage(size_t size, size_t align) {
    return dataAllocator_.allocate(size, align);
}

void ModuleCodegen::finalizeFreestandingHeapBase() {
    // Round up the post-data top to 8 bytes so callers can rely on
    // __heap_base being i64-aligned. Patch the slot reserved in setupGlobals.
    size_t top = dataAllocator_.currentTop();
    top = (top + 7u) & ~size_t{7};
    if (heapBaseGlobalIdx_ >= 0
        && heapBaseGlobalIdx_ < (int)module_.globals.size()) {
        module_.globals[heapBaseGlobalIdx_].init =
            WasmVM::Instr::I64_const{(WasmVM::i64_t)top};
    }
}

void ModuleCodegen::setupMemory() {
    WasmVM::MemType memTy;
    memTy.min = 1;
    memTy.is64 = true;
    memTy.max = std::nullopt;

    if (compileMode_ == CompileMode::Linkable) {
        // Linkable mode: import both memories from the env module so multiple
        // linked TUs share the same heap (mem[0]) and shadow stack (mem[1]).
        // The linker's synthetic crt0 (M2-L6) provides them.
        WasmVM::WasmImport heap;
        heap.module = "env";
        heap.name = "__linear_memory";
        heap.desc = memTy;
        module_.imports.push_back(heap);

        WasmVM::WasmImport shadow;
        shadow.module = "env";
        shadow.name = "__stack_memory";
        shadow.desc = memTy;
        module_.imports.push_back(shadow);
        return;
    }

    // Freestanding: self-contained — define mem[0] and mem[1] locally.
    module_.mems.push_back(memTy);
    module_.mems.push_back(memTy);
}

void ModuleCodegen::setupGlobals() {
    WasmVM::GlobalType spType{WasmVM::GlobalType::variable, WasmVM::ValueType::i64};
    WasmVM::GlobalType heapBaseType{WasmVM::GlobalType::constant, WasmVM::ValueType::i64};

    if (compileMode_ == CompileMode::Linkable) {
        // Linkable mode: import $__stack_pointer (mut i64) and $__heap_base
        // (const i64) from env. Both initial values are set by the linker's
        // crt0 (M2-L6) after the merged memory layout is known. Imported
        // globals occupy the low global-index slots: __stack_pointer = 0,
        // __heap_base = 1.
        WasmVM::WasmImport spImp;
        spImp.module = "env";
        spImp.name = "__stack_pointer";
        spImp.desc = spType;
        module_.imports.push_back(spImp);
        stackPtrGlobalIdx_ = 0;

        WasmVM::WasmImport hbImp;
        hbImp.module = "env";
        hbImp.name = "__heap_base";
        hbImp.desc = heapBaseType;
        module_.imports.push_back(hbImp);
        heapBaseGlobalIdx_ = 1;
        return;
    }

    // Freestanding: define the stack pointer locally with M1's standalone
    // init value at global 0. Reserve __heap_base at global 1 *now* (init
    // patched in finalizeFreestandingHeapBase once the data layout is final)
    // so its index is stable even when static-local guard globals are
    // appended during secondPass.
    WasmVM::WasmGlobal stackPointerGlobal;
    stackPointerGlobal.type = spType;
    stackPointerGlobal.init = WasmVM::Instr::I64_const{0x10000};
    module_.globals.push_back(stackPointerGlobal);
    stackPtrGlobalIdx_ = 0;

    WasmVM::WasmGlobal heapBaseGlobal;
    heapBaseGlobal.type = heapBaseType;
    heapBaseGlobal.init = WasmVM::Instr::I64_const{0}; // patched in finalize
    module_.globals.push_back(heapBaseGlobal);
    heapBaseGlobalIdx_ = 1;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

WasmVM::index_t ModuleCodegen::internFuncType(const WasmVM::FuncType& ft) {
    for (WasmVM::index_t i = 0; i < (WasmVM::index_t)module_.types.size(); ++i) {
        if (module_.types[i] == ft) return i;
    }
    module_.types.push_back(ft);
    return (WasmVM::index_t)(module_.types.size() - 1);
}

std::string ModuleCodegen::getFuncName(const wvmcc::parser::DeclaratorPtr& decl) const {
    if (!decl) return "";
    if (!decl->id.name.empty()) return decl->id.name;
    if (decl->inner) return getFuncName(*decl->inner);
    return "";
}

// True iff `params` is the C `(void)` parameter list (single unnamed `void`
// parameter), which the standard treats as zero parameters.
static bool isVoidParamList(const std::vector<wvmcc::parser::Parameter>& params) {
    if (params.size() != 1) return false;
    const auto& p = params[0];
    if (p.declarator) return false; // a named/typed declarator means a real param
    for (const auto& ts : p.specifiers.typeSpecifiers) {
        if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
            && ts.simple.size() == 1
            && ts.simple[0] == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void) {
            return true;
        }
    }
    return false;
}

// Build the return TypeNode for a function, correctly applying pointer qualifiers from
// the declarator chain that appear before the function-kind declarator.
static wvmcc::parser::TypeNodePtr buildReturnTypeNode(
    const wvmcc::parser::DeclarationSpecifiers& specs,
    const wvmcc::parser::DeclaratorPtr& decl,
    const wvmcc::parser::Semantic& semantic) {

    // canonicalTypeRepr resolves typedef-named base types (e.g. `ssize_t`
    // → long → i64) so the signature matches the function body's view.
    auto baseType = semantic.canonicalTypeRepr(specs, nullptr);

    // The parser builds the declarator chain outer→inner as
    //   [trailing-suffix(Function/Array)] → Identifier → [leading `*`s]
    // For a function definition the outermost layer is Function and the
    // pointer/array qualifiers that decorate the RETURN TYPE sit BELOW the
    // Identifier (they came from the `*` prefix that the parser consumed
    // before the identifier). Walk to the identifier, then collect
    // everything after it.
    std::vector<wvmcc::parser::Declarator::Kind> quals;
    bool sawIdentifier = false;
    for (auto cur = decl; cur;
         cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (sawIdentifier) {
            if (cur->kind == wvmcc::parser::Declarator::Kind::Pointer
                || cur->kind == wvmcc::parser::Declarator::Kind::Array) {
                quals.push_back(cur->kind);
            }
        }
        if (cur->kind == wvmcc::parser::Declarator::Kind::Identifier) {
            sawIdentifier = true;
        }
    }
    // Apply qualifiers from innermost-to-outermost.
    for (auto it = quals.rbegin(); it != quals.rend(); ++it) {
        if (*it == wvmcc::parser::Declarator::Kind::Pointer) {
            auto ptrNode = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
            ptrNode->kind = wvmcc::parser::TypeNode::Kind::Pointer;
            ptrNode->pointee = baseType;
            baseType = ptrNode;
        }
    }
    return baseType;
}

WasmVM::FuncType ModuleCodegen::buildFuncTypeFromDef(const wvmcc::parser::FunctionDefPtr& funcDef) const {
    WasmVM::FuncType ft;

    // Hosted-environment override: main() always has the canonical Wasm
    // signature regardless of how its parameter list is spelled in C.
    //   main()              -> () -> i32
    //   main(int, char**)   -> (i32, i64) -> i32
    if (funcDef && getFuncName(funcDef->declarator) == "main") {
        ft.results.push_back(WasmVM::ValueType::i32);
        if (funcDef->params.size() == 2) {
            ft.params = {WasmVM::ValueType::i32, WasmVM::ValueType::i64};
        }
        return ft;
    }

    auto retType = buildReturnTypeNode(funcDef->specifiers, funcDef->declarator, semantic_);
    bool isVoid = retType && retType->kind == wvmcc::parser::TypeNode::Kind::Builtin
                  && !retType->simple.empty()
                  && retType->simple[0] == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void;
    bool isStructRet = retType && (retType->kind == wvmcc::parser::TypeNode::Kind::Struct
                                   || retType->kind == wvmcc::parser::TypeNode::Kind::Union);

    if (isStructRet) {
        ft.params.push_back(WasmVM::ValueType::i64); // hidden sret pointer
    } else if (!isVoid) {
        ft.results.push_back(retType ? typeMap_.toWasmType(retType) : WasmVM::ValueType::i32);
    }

    if (!isVoidParamList(funcDef->params)) {
        for (const auto& param : funcDef->params) {
            auto paramType = semantic_.canonicalTypeRepr(param.specifiers, param.declarator);
            if (!paramType) {
                ft.params.push_back(WasmVM::ValueType::i32);
            } else if (paramType->kind == wvmcc::parser::TypeNode::Kind::Struct
                       || paramType->kind == wvmcc::parser::TypeNode::Kind::Union) {
                ft.params.push_back(WasmVM::ValueType::i64); // pointer to caller's struct copy
            } else {
                ft.params.push_back(typeMap_.toWasmType(paramType));
            }
        }
    }

    if (funcDef->isVariadic) {
        ft.params.push_back(WasmVM::ValueType::i64); // hidden trailing va_args spill-base pointer
    }

    return ft;
}

WasmVM::FuncType ModuleCodegen::buildFuncTypeFromDecl(const wvmcc::parser::DeclarationPtr& decl) const {
    WasmVM::FuncType ft;

    auto retType = buildReturnTypeNode(decl->specifiers, decl->declarator, semantic_);
    bool isVoid = retType && retType->kind == wvmcc::parser::TypeNode::Kind::Builtin
                  && !retType->simple.empty()
                  && retType->simple[0] == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void;
    bool isStructRet = retType && (retType->kind == wvmcc::parser::TypeNode::Kind::Struct
                                   || retType->kind == wvmcc::parser::TypeNode::Kind::Union);

    if (isStructRet) {
        ft.params.push_back(WasmVM::ValueType::i64);
    } else if (!isVoid) {
        ft.results.push_back(retType ? typeMap_.toWasmType(retType) : WasmVM::ValueType::i32);
    }

    if (decl->declarator && decl->declarator->kind == wvmcc::parser::Declarator::Kind::Function) {
        const auto& params = decl->declarator->function.params;
        if (!isVoidParamList(params)) {
            for (const auto& param : params) {
                auto paramType = semantic_.canonicalTypeRepr(param.specifiers, param.declarator);
                if (!paramType) {
                    ft.params.push_back(WasmVM::ValueType::i32);
                } else if (paramType->kind == wvmcc::parser::TypeNode::Kind::Struct
                           || paramType->kind == wvmcc::parser::TypeNode::Kind::Union) {
                    ft.params.push_back(WasmVM::ValueType::i64);
                } else {
                    ft.params.push_back(typeMap_.toWasmType(paramType));
                }
            }
        }
        if (decl->declarator->function.isVariadic) {
            ft.params.push_back(WasmVM::ValueType::i64);
        }
    }

    return ft;
}

// ---------------------------------------------------------------------------
// First pass: register types and function symbols, add imports
// ---------------------------------------------------------------------------

void ModuleCodegen::firstPass(const wvmcc::parser::TranslationUnitPtr& tu) {
    if (!tu) return;
    // Pre-scan: collect names of functions DEFINED in this TU. A bare
    // prototype for a name that's defined later in the same TU must not
    // become an import — otherwise the resulting module both imports and
    // exports the function under the same name, which the linker (and
    // any sensible reader) chokes on.
    std::unordered_set<std::string> definedInTU;
    for (const auto& external : tu->externals) {
        if (!external) continue;
        if (auto fd = std::get_if<wvmcc::parser::FunctionDefPtr>(&external->decl)) {
            if (*fd) definedInTU.insert(getFuncName((*fd)->declarator));
        }
    }
    for (const auto& external : tu->externals) {
        if (!external) continue;
        // Skip prototype declarations for names defined later in this TU.
        if (auto d = std::get_if<wvmcc::parser::DeclarationPtr>(&external->decl)) {
            if (*d && (*d)->declarator
                && (*d)->declarator->kind == wvmcc::parser::Declarator::Kind::Function
                && definedInTU.count(getFuncName((*d)->declarator))) {
                continue;
            }
        }
        registerExternalDecl(external);
    }

    // #77: register runtime globals + file-scope variable definitions as
    // symbols so function bodies (emitted in secondPass) can resolve them.
    registerGlobalVars(tu);
}

void ModuleCodegen::registerExternalDecl(const wvmcc::parser::ExternalDeclPtr& decl) {
    if (!decl) return;
    std::visit([this](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, wvmcc::parser::FunctionDefPtr>) {
            registerFunctionDef(item);
        } else if constexpr (std::is_same_v<T, wvmcc::parser::DeclarationPtr>) {
            if (item && item->declarator
                && item->declarator->kind == wvmcc::parser::Declarator::Kind::Function) {
                registerFunctionDeclaration(item);
            }
        }
    }, decl->decl);
}

// A function is no-return if it carries the C11 `_Noreturn` function specifier
// or GNU `__attribute__((noreturn))`. Call sites use this to emit a trailing
// `unreachable`, so a non-void caller that ends in e.g. `exit(...)` doesn't
// fall through to its `end` with an empty operand stack (a validation error).
static bool isNoReturn(const wvmcc::parser::DeclarationSpecifiers& specs,
                       const std::vector<wvmcc::parser::GnuAttribute>& attrs) {
    if (specs.hasFuncSpec(wvmcc::parser::FunctionSpecifier::NoReturn)) return true;
    for (const auto& a : attrs) {
        if (a.name == "noreturn" || a.name == "__noreturn__") return true;
    }
    return false;
}

void ModuleCodegen::registerFunctionDef(const wvmcc::parser::FunctionDefPtr& funcDef) {
    if (!funcDef) return;

    auto ft = buildFuncTypeFromDef(funcDef);
    WasmVM::index_t typeidx = internFuncType(ft);

    std::string name = getFuncName(funcDef->declarator);
    funcTypeIdx_[name] = typeidx;

    FuncSymbol sym;
    sym.type = buildReturnTypeNode(funcDef->specifiers, funcDef->declarator, semantic_);
    sym.funcIndex = nextFuncIndex_++;
    sym.isImport = false;
    sym.isVariadic = funcDef->isVariadic;
    sym.noReturn = isNoReturn(funcDef->specifiers, funcDef->gnuAttributes);
    sym.namedParamCount = isVoidParamList(funcDef->params)
                              ? 0
                              : static_cast<int>(funcDef->params.size());
    if (!isVoidParamList(funcDef->params)) {
        for (const auto& p : funcDef->params) {
            auto pt = semantic_.canonicalTypeRepr(p.specifiers, p.declarator);
            sym.paramTypes.push_back(
                pt ? typeMap_.toWasmType(pt) : WasmVM::ValueType::i32);
        }
    }
    symbolTable_.defineFunction(name, sym);

    if (name == "main") {
        mainFuncIndex_ = sym.funcIndex;
        // M2-L6: linkable-mode TUs export `main` as a hint to the linker,
        // which uses it to wire the crt0 start function. Freestanding mode
        // exports main via the start wrapper instead.
        if (compileMode_ == CompileMode::Linkable) {
            WasmVM::WasmExport ex;
            ex.name = "main";
            ex.desc = WasmVM::WasmExport::DescType::func;
            ex.index = (WasmVM::index_t)sym.funcIndex;
            module_.exports.push_back(ex);
        }
    }

    // M2-C: explicit export opt-in via GNU attributes. `static` functions
    // are always internal (C standard); for other functions, exports are
    // additive — neither this nor `main`'s start-wrapper export conflict.
    const bool isStatic = funcDef->specifiers.hasStorage(
        wvmcc::parser::StorageClass::Static);
    if (!isStatic) {
        std::optional<std::string> exportName;
        for (const auto& attr : funcDef->gnuAttributes) {
            if (attr.name == "export_name" && !attr.stringArgs.empty()) {
                exportName = attr.stringArgs[0];
            } else if (attr.name == "visibility" && !attr.stringArgs.empty()
                       && attr.stringArgs[0] == "default") {
                if (!exportName) exportName = name;
            }
        }
        // M2-L*: linkable mode auto-exports every non-static function so the
        // linker's name-based resolver can wire cross-TU calls. Without this,
        // the only externally-visible names would be those tagged with
        // export_name/visibility attributes — which is fine for freestanding
        // but useless when this object is destined for libc.a.
        if (!exportName && compileMode_ == CompileMode::Linkable && name != "main") {
            exportName = name;
        }
        if (exportName) {
            WasmVM::WasmExport ex;
            ex.name = *exportName;
            ex.desc = WasmVM::WasmExport::DescType::func;
            ex.index = (WasmVM::index_t)sym.funcIndex;
            module_.exports.push_back(ex);
        }
    }
}

void ModuleCodegen::registerFunctionDeclaration(const wvmcc::parser::DeclarationPtr& decl) {
    if (!decl || !decl->declarator) return;

    auto ft = buildFuncTypeFromDecl(decl);
    WasmVM::index_t typeidx = internFuncType(ft);

    std::string name = getFuncName(decl->declarator);
    funcTypeIdx_[name] = typeidx;

    std::string importModule = "env";
    std::string importName = name;
    for (const auto& attr : decl->gnuAttributes) {
        if (attr.name == "import_module" && !attr.stringArgs.empty()) {
            importModule = attr.stringArgs[0];
        } else if (attr.name == "import_name" && !attr.stringArgs.empty()) {
            importName = attr.stringArgs[0];
        }
    }

    WasmVM::WasmImport imp;
    imp.module = importModule;
    imp.name = importName;
    imp.desc = typeidx;
    module_.imports.push_back(imp);

    FuncSymbol sym;
    // Carry the structured return type so call-site type queries
    // (getExprTypeNode for K::Call) see the real `void *`/etc., not the
    // i32 fallback that was used before — that fallback corrupted casts
    // applied to imported-function return values.
    sym.type = buildReturnTypeNode(decl->specifiers, decl->declarator, semantic_);
    sym.funcIndex = nextFuncIndex_++;
    sym.isImport = true;
    sym.noReturn = isNoReturn(decl->specifiers, decl->gnuAttributes);
    if (decl->declarator->kind == wvmcc::parser::Declarator::Kind::Function) {
        sym.isVariadic = decl->declarator->function.isVariadic;
        const auto& dparams = decl->declarator->function.params;
        sym.namedParamCount = isVoidParamList(dparams)
                                  ? 0
                                  : static_cast<int>(dparams.size());
        // Wasm value types for each named C parameter — drives call-site
        // argument coercion (i32 → i64, etc.) so we don't trip validation
        // when an `int` literal is passed where the callee expects i64.
        if (!isVoidParamList(dparams)) {
            for (const auto& p : dparams) {
                auto pt = semantic_.canonicalTypeRepr(p.specifiers, p.declarator);
                sym.paramTypes.push_back(
                    pt ? typeMap_.toWasmType(pt) : WasmVM::ValueType::i32);
            }
        }
    }
    symbolTable_.defineFunction(name, sym);
}

// ---------------------------------------------------------------------------
// #79: function-pointer support. Every function whose address is taken (via
// `&f` or a bare-name decay) interns a funcref-table slot during body emission
// (internFuncTableSlot). After secondPass we materialize a funcref table sized
// to those slots plus an active element segment populating table[slot] =
// ref.func(f). call_indirect dispatches through this table; the linker merges
// per-TU tables into one when combining objects.
// ---------------------------------------------------------------------------

void ModuleCodegen::emitFuncTable() {
    // A module that only *calls* through function pointers (e.g. qsort taking a
    // `compar` parameter) takes no function address of its own, so it has no
    // slots — yet its call_indirect still needs table 0 to exist to validate.
    // Detect that case and emit an empty table the linker will later grow.
    if (funcTableSlots_.empty()) {
        bool usesCallIndirect = false;
        for (const auto& f : module_.funcs) {
            for (const auto& instr : f.body) {
                if (instr.opcode == WasmVM::Opcode::Call_indirect) {
                    usesCallIndirect = true;
                    break;
                }
            }
            if (usesCallIndirect) break;
        }
        if (!usesCallIndirect) return;
        WasmVM::TableType t;
        t.limits.min = 0;
        t.limits.max = std::nullopt;
        t.limits.is64 = false;
        t.reftype = WasmVM::RefType::funcref;
        module_.tables.push_back(t);
        return;
    }

    const size_t nslots = funcTableSlots_.size();

    // Define a local funcref table 0 sized to fit every slot. Objects compiled
    // with -c carry their own table so they validate standalone; the linker
    // concatenates them.
    WasmVM::TableType t;
    t.limits.min = (WasmVM::offset_t)nslots;
    t.limits.max = (WasmVM::offset_t)nslots;
    t.limits.is64 = false;
    t.reftype = WasmVM::RefType::funcref;
    module_.tables.push_back(t);

    // Active element segment: table[slot] = ref.func(funcIndex). Order the
    // entries by slot so elemlist[i] populates slot i from offset 0.
    std::vector<std::string> bySlot(nslots);
    for (const auto& [name, slot] : funcTableSlots_) bySlot[slot] = name;

    WasmVM::WasmElem elem;
    elem.type = WasmVM::RefType::funcref;
    elem.mode.type = WasmVM::WasmElem::ElemMode::Mode::active;
    elem.mode.tableidx = 0;
    elem.mode.offset = WasmVM::Instr::I32_const{0};
    for (const auto& name : bySlot) {
        auto sym = symbolTable_.lookupFunction(name);
        WasmVM::index_t fidx = sym ? (WasmVM::index_t)sym->funcIndex : 0;
        elem.elemlist.push_back(WasmVM::Instr::Ref_func{fidx});
    }
    module_.elems.push_back(std::move(elem));
}

// ---------------------------------------------------------------------------
// Second pass: generate function bodies
// ---------------------------------------------------------------------------

void ModuleCodegen::secondPass(const wvmcc::parser::TranslationUnitPtr& tu) {
    if (!tu) return;
    for (const auto& external : tu->externals) {
        if (!external) continue;
        std::visit([this](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, wvmcc::parser::FunctionDefPtr>) {
                emitFunctionDefinition(item);
            }
        }, external->decl);
    }
    emitStringLiterals();
}

void ModuleCodegen::emitFunctionDefinition(const wvmcc::parser::FunctionDefPtr& funcDef) {
    if (!funcDef) return;

    FunctionCodegen funcCodegen(typeMap_, symbolTable_, &dataAllocator_, this, &semantic_);
    auto wasmFunc = funcCodegen.generate(funcDef, semantic_);

    // Surface any per-function codegen diagnostics (e.g. unimplemented
    // constructs, undeclared identifiers) up to the driver.
    const auto& fcDiags = funcCodegen.getDiagnostics();
    diagnostics_.insert(diagnostics_.end(), fcDiags.begin(), fcDiags.end());

    auto ft = buildFuncTypeFromDef(funcDef);
    wasmFunc.typeidx = internFuncType(ft);

    // M2-E: collect data-pointer sites from this function into module-level
    // relocation records. Each unique data address becomes a data symbol.
    if (compileMode_ == CompileMode::Linkable) {
        size_t codeFuncIdx = module_.funcs.size();
        for (const auto& site : funcCodegen.getDataPtrSites()) {
            // Dedup data symbols by address.
            size_t symIdx = (size_t)-1;
            for (size_t i = 0; i < dataSymbols_.size(); ++i) {
                if (dataSymbols_[i].address == site.address) { symIdx = i; break; }
            }
            if (symIdx == (size_t)-1) {
                symIdx = dataSymbols_.size();
                dataSymbols_.push_back({
                    std::string("$str.") + std::to_string(site.address),
                    site.address,
                });
            }
            relocations_.push_back({codeFuncIdx, site.instrIdx, symIdx, 0});
        }
        // #79: function-pointer sites — i64.const (tag | slot) constants whose
        // embedded slot the linker rebases when merging per-TU funcref tables.
        for (const auto& site : funcCodegen.getFuncPtrSites()) {
            funcPtrRelocs_.push_back({codeFuncIdx, site.instrIdx});
        }
    }

    module_.funcs.push_back(wasmFunc);
}

// LANG-6.6-06: function-pointer tag, mirroring FunctionCodegen::kFuncPtrTag
// (high nibble 0xF marks a tagged funcref-table slot). Kept in sync by value.
static constexpr std::int64_t kModFuncPtrTag = (std::int64_t)0xF << 60;

// Strip top-level cv-qualifier wrappers to reach the real type shape.
static wvmcc::parser::TypeNodePtr unwrapQual(wvmcc::parser::TypeNodePtr t) {
    using TK = wvmcc::parser::TypeNode::Kind;
    while (t && t->kind == TK::Qualified && t->pointee) t = t->pointee;
    return t;
}

// LANG-6.6-06: evaluate the address of a designator (the operand of `&`, or an
// array/struct sub-object reached via [], ., *). Returns the mem[0] address and
// the designated type, or a function designator. Nullopt if not a link-time
// constant designator.
std::optional<ModuleCodegen::Designator>
ModuleCodegen::evalDesignator(const wvmcc::parser::ExprPtr& e) {
    using K = wvmcc::parser::Expr::Kind;
    using TK = wvmcc::parser::TypeNode::Kind;
    if (!e) return std::nullopt;

    if (e->kind == K::Ident) {
        const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*e);
        if (auto fs = symbolTable_.lookupFunction(id.name)) {
            Designator d;
            d.isFunc = true;
            d.funcSlot = internFuncTableSlot(id.name);
            return d;
        }
        if (auto sym = symbolTable_.lookup(id.name)) {
            if (auto* gm = std::get_if<GlobalMem>(&*sym)) {
                if (gm->isImport) return std::nullopt; // address resolved at link
                Designator d;
                d.addr = (std::int64_t)gm->address;
                d.type = unwrapQual(gm->type);
                return d;
            }
        }
        return std::nullopt;
    }

    if (e->kind == K::Member) {
        const auto& m = static_cast<const wvmcc::parser::MemberExpr&>(*e);
        if (m.isArrow) return std::nullopt; // p->m needs a load — not constant
        auto base = evalDesignator(m.base);
        if (!base || base->isFunc || !base->type) return std::nullopt;
        auto st = unwrapQual(base->type);
        if (!st || (st->kind != TK::Struct && st->kind != TK::Union))
            return std::nullopt;
        auto fieldType = typeMap_.getFieldType(st, m.member);
        if (!fieldType) return std::nullopt;
        Designator d;
        d.addr = base->addr + (std::int64_t)typeMap_.getFieldOffset(st, m.member);
        d.type = unwrapQual(fieldType);
        return d;
    }

    if (e->kind == K::Index) {
        const auto& ix = static_cast<const wvmcc::parser::IndexExpr&>(*e);
        // One operand designates an array (or address-constant pointer), the
        // other is an integer constant.
        const wvmcc::parser::ExprPtr* arrE = nullptr;
        const wvmcc::parser::ExprPtr* idxE = nullptr;
        auto baseDes = evalDesignator(ix.base);
        if (baseDes && baseDes->type && unwrapQual(baseDes->type)->kind == TK::Array) {
            arrE = &ix.base; idxE = &ix.index;
        } else {
            auto idxDes = evalDesignator(ix.index);
            if (idxDes && idxDes->type && unwrapQual(idxDes->type)->kind == TK::Array) {
                arrE = &ix.index; idxE = &ix.base;     // commuted: k[arr]
            }
        }
        if (arrE) {
            auto arr = evalDesignator(*arrE);
            if (!arr || !arr->type) return std::nullopt;
            auto at = unwrapQual(arr->type);
            auto elem = at->element ? unwrapQual(at->element) : nullptr;
            if (!elem) return std::nullopt;
            auto k = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(*idxE);
            if (!k) return std::nullopt;
            Designator d;
            d.addr = arr->addr + (std::int64_t)(*k) * (std::int64_t)typeMap_.byteSize(elem);
            d.type = elem;
            return d;
        }
        // Indexing an address-constant pointer value: `(arr + 1)[2]`, `p[2]`
        // where p decays from an address constant.
        if (auto ac = evalAddressConstInit(ix.base);
            ac && !ac->isFuncPtr && ac->pointee) {
            auto k = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(ix.index);
            if (!k) return std::nullopt;
            Designator d;
            d.addr = ac->value + (std::int64_t)(*k) * (std::int64_t)typeMap_.byteSize(ac->pointee);
            d.type = ac->pointee;
            return d;
        }
        return std::nullopt;
    }

    if (e->kind == K::Unary) {
        const auto& u = static_cast<const wvmcc::parser::UnaryExpr&>(*e);
        if (u.op == "*") {
            // *P designates the object P points at.
            if (auto ac = evalAddressConstInit(u.rhs);
                ac && !ac->isFuncPtr && ac->pointee) {
                Designator d;
                d.addr = ac->value;
                d.type = ac->pointee;
                return d;
            }
        }
        return std::nullopt;
    }

    return std::nullopt;
}

// LANG-6.6-06: evaluate a pointer-valued address constant (6.6p9).
std::optional<ModuleCodegen::AddrConst>
ModuleCodegen::evalAddressConstInit(const wvmcc::parser::ExprPtr& e) {
    using K = wvmcc::parser::Expr::Kind;
    using TK = wvmcc::parser::TypeNode::Kind;
    if (!e) return std::nullopt;

    // A pointer cast preserves the address value (LP64: all pointers are i64).
    if (e->kind == K::Cast) {
        const auto& c = static_cast<const wvmcc::parser::CastExpr&>(*e);
        return evalAddressConstInit(c.expr);
    }

    if (e->kind == K::Unary) {
        const auto& u = static_cast<const wvmcc::parser::UnaryExpr&>(*e);
        if (u.op == "&") {
            auto d = evalDesignator(u.rhs);
            if (!d) return std::nullopt;
            AddrConst ac;
            if (d->isFunc) {
                ac.value = kModFuncPtrTag | (std::int64_t)d->funcSlot;
                ac.isFuncPtr = true;
            } else {
                ac.value = d->addr;
                ac.pointee = d->type;
            }
            return ac;
        }
        if (u.op == "+") return evalAddressConstInit(u.rhs);
        return std::nullopt;
    }

    if (e->kind == K::Ident) {
        const auto& id = static_cast<const wvmcc::parser::IdentifierExpr&>(*e);
        if (auto fs = symbolTable_.lookupFunction(id.name)) {
            AddrConst ac;
            ac.value = kModFuncPtrTag | (std::int64_t)internFuncTableSlot(id.name);
            ac.isFuncPtr = true;
            return ac;
        }
        if (auto sym = symbolTable_.lookup(id.name)) {
            if (auto* gm = std::get_if<GlobalMem>(&*sym)) {
                if (gm->isImport) return std::nullopt;
                auto t = unwrapQual(gm->type);
                if (t && t->kind == TK::Array) {       // array-to-pointer decay
                    AddrConst ac;
                    ac.value = (std::int64_t)gm->address;
                    ac.pointee = t->element ? unwrapQual(t->element) : nullptr;
                    return ac;
                }
            }
        }
        return std::nullopt;
    }

    // Pointer ± integer constant (array decay base or another address const).
    if (e->kind == K::Binary) {
        const auto& b = static_cast<const wvmcc::parser::BinaryExpr&>(*e);
        if (b.op != "+" && b.op != "-") return std::nullopt;
        // Identify the pointer operand and the integer operand.
        auto lp = evalAddressConstInit(b.lhs);
        if (lp && !lp->isFuncPtr && lp->pointee) {
            auto k = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(b.rhs);
            if (!k) return std::nullopt;
            std::int64_t scaled = (std::int64_t)(*k) * (std::int64_t)typeMap_.byteSize(lp->pointee);
            AddrConst ac = *lp;
            ac.value = lp->value + (b.op == "+" ? scaled : -scaled);
            return ac;
        }
        if (b.op == "+") { // n + ptr
            auto rp = evalAddressConstInit(b.rhs);
            if (rp && !rp->isFuncPtr && rp->pointee) {
                auto k = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(b.lhs);
                if (!k) return std::nullopt;
                AddrConst ac = *rp;
                ac.value = rp->value + (std::int64_t)(*k) * (std::int64_t)typeMap_.byteSize(rp->pointee);
                return ac;
            }
        }
        return std::nullopt;
    }

    return std::nullopt;
}

// #77: encode a constant scalar initializer expression into `out` (little-
// endian, `size` bytes). Returns false if the initializer is not a simple
// compile-time constant we can encode (caller then leaves storage zeroed).
static bool encodeScalarConstInit(const wvmcc::parser::ExprPtr& expr,
                                  size_t size, bool isFloat,
                                  std::vector<std::byte>& out) {
    using K = wvmcc::parser::Expr::Kind;
    if (!expr) return false;
    // Peel a leading unary minus on a numeric literal.
    if (isFloat) {
        double dv = 0.0;
        bool neg = false;
        const wvmcc::parser::Expr* e = expr.get();
        if (e->kind == K::Unary) {
            const auto& u = static_cast<const wvmcc::parser::UnaryExpr&>(*e);
            if (u.op == "-") { neg = true; e = u.rhs.get(); }
            else if (u.op == "+") { e = u.rhs.get(); }
        }
        if (e && e->kind == K::Float)
            dv = static_cast<const wvmcc::parser::FloatLiteral&>(*e).value;
        else if (e && e->kind == K::Integer)
            dv = (double)static_cast<const wvmcc::parser::IntegerLiteral&>(*e).value;
        else return false;
        if (neg) dv = -dv;
        if (size == 4) { float f = (float)dv; std::memcpy(out.data(), &f, 4); }
        else           { std::memcpy(out.data(), &dv, 8); }
        return true;
    }
    // Fold any integer constant expression (literals, unary +/-, |, <<, &,
    // arithmetic, enum constants, …) — e.g. `_F_WRITE | _F_LINEBUF`.
    auto folded = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(expr);
    if (!folded) return false;
    auto uv = (std::uint64_t)(std::int64_t)*folded;
    for (size_t i = 0; i < size && i < 8; ++i)
        out[i] = std::byte((uv >> (8 * i)) & 0xFF);
    return true;
}

bool ModuleCodegen::encodeConstInit(const wvmcc::parser::TypeNodePtr& type,
                                    const wvmcc::parser::InitializerPtr& init,
                                    size_t base, std::vector<std::byte>& out) {
    using TK = wvmcc::parser::TypeNode::Kind;
    if (!type || !init) return false;

    // Unwrap a top-level cv-qualifier wrapper to reach the real shape.
    auto t = type;
    while (t && t->kind == TK::Qualified && t->pointee) t = t->pointee;
    if (!t) return false;

    // Scalar (Expr) initializer — possibly brace-wrapped (`int x = {5};`).
    if (init->kind == wvmcc::parser::Initializer::Kind::Expr) {
        if (!init->expr) return false;
        if (t->kind == TK::Array || t->kind == TK::Struct || t->kind == TK::Union)
            return false; // aggregate initialized by a scalar expr — unsupported
        size_t sz = typeMap_.byteSize(t);
        if (sz == 0) sz = 4;
        bool isFloat = false;
        if (t->kind == TK::Builtin) {
            using STS = wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier;
            for (auto s : t->simple)
                if (s == STS::Float || s == STS::Double) isFloat = true;
        }
        if (base + sz > out.size()) return false;
        std::vector<std::byte> tmp(sz, std::byte{0});
        if (encodeScalarConstInit(init->expr, sz, isFloat, tmp)) {
            for (size_t i = 0; i < sz; ++i) out[base + i] = tmp[i];
            return true;
        }
        // LANG-6.6-06: address constant (&obj, array/function decay, &arr[k],
        // arr + k, &s.m). Bake the link-time i64 value and record the site so
        // the linker rebases it when this TU's data / funcref table is merged.
        if (auto ac = evalAddressConstInit(init->expr)) {
            auto uv = (std::uint64_t)ac->value;
            for (size_t i = 0; i < sz && i < 8; ++i)
                out[base + i] = std::byte((uv >> (8 * i)) & 0xFF);
            pendingAddrSites_.emplace_back(base, ac->isFuncPtr);
            return true;
        }
        return false;
    }

    // Braced list initializer.
    const auto& clauses = init->clauses;
    if (t->kind == TK::Array) {
        auto elemType = t->element;
        if (!elemType) return false;
        size_t elemSize = typeMap_.byteSize(elemType);
        if (elemSize == 0) return false;
        // Element index the next clause initializes; an index designator `[k]`
        // repositions it (6.7.9p17,p18), and a later clause for the same index
        // overrides the earlier one (p19) — which falls out naturally because
        // `out` is simply rewritten. Nested designators (`[k].m`) in a global
        // initializer are not yet supported.
        size_t cursor = 0;
        for (const auto& cl : clauses) {
            if (!cl.designators.empty()) {
                if (cl.designators.size() != 1
                    || cl.designators.front().kind != wvmcc::parser::Designator::Kind::Index
                    || !cl.designators.front().index)
                    return false;
                auto vi = wvmcc::parser::ConstExprEvaluator::evalIntegerConstantExpr(
                    cl.designators.front().index.value());
                if (!vi.has_value()) return false;
                cursor = (size_t)*vi;
            }
            if (cl.init && !encodeConstInit(elemType, cl.init, base + cursor * elemSize, out))
                return false;
            cursor += 1;
        }
        return true;
    }
    if (t->kind == TK::Struct || t->kind == TK::Union) {
        auto names = typeMap_.getOrderedFieldNames(t);
        size_t limit = (t->kind == TK::Union) ? 1 : names.size();
        // Field index the next clause initializes; a member designator `.m`
        // repositions it to that field (6.7.9p17,p18). Nested designators
        // (`.m.n`) in a global initializer are not yet supported.
        size_t cursor = 0;
        for (const auto& cl : clauses) {
            if (!cl.designators.empty()) {
                if (cl.designators.size() != 1
                    || cl.designators.front().kind != wvmcc::parser::Designator::Kind::Member)
                    return false;
                const std::string& m = cl.designators.front().member;
                size_t fi = 0; bool found = false;
                for (; fi < names.size(); ++fi) if (names[fi] == m) { found = true; break; }
                if (!found) return false;
                cursor = fi;
            }
            if (cursor >= names.size() || cursor >= limit) break;
            if (cl.init) {
                auto fieldType   = typeMap_.getFieldType(t, names[cursor]);
                size_t fieldOff  = typeMap_.getFieldOffset(t, names[cursor]);
                if (!fieldType) return false;
                if (!encodeConstInit(fieldType, cl.init, base + fieldOff, out))
                    return false;
            }
            cursor += 1;
        }
        return true;
    }
    // Scalar with a single-element brace list: `int x = {5};`
    if (!clauses.empty() && clauses[0].init)
        return encodeConstInit(t, clauses[0].init, base, out);
    return false;
}

void ModuleCodegen::registerGlobalVars(const wvmcc::parser::TranslationUnitPtr& tu) {
    // Runtime globals: C references to these names resolve to global.get /
    // global.set on the imported/defined Wasm globals reserved in
    // setupGlobals(). __heap_base is const (read-only); __stack_pointer is
    // mutable. Their C type is `unsigned long` (i64 pointer-width).
    auto i64Type = []() {
        auto tn = wvmcc::parser::make_ast<wvmcc::parser::TypeNode>();
        tn->kind = wvmcc::parser::TypeNode::Kind::Builtin;
        tn->simple.push_back(
            wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Unsigned);
        tn->simple.push_back(
            wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Long);
        return tn;
    };
    if (heapBaseGlobalIdx_ >= 0) {
        GlobalScalar gs; gs.type = i64Type(); gs.isMutable = false;
        gs.globalIndex = heapBaseGlobalIdx_;
        symbolTable_.define("__heap_base", gs);
    }
    if (stackPtrGlobalIdx_ >= 0) {
        GlobalScalar gs; gs.type = i64Type(); gs.isMutable = true;
        gs.globalIndex = stackPtrGlobalIdx_;
        symbolTable_.define("__stack_pointer", gs);
    }

    if (!tu) return;
    // Two passes so a real definition always wins over an `extern` declaration
    // of the same name, regardless of source order. A header's
    // `extern FILE *stdout;` would otherwise register `stdout` as an imported
    // address-global, and the later defining `FILE *stdout = &__stdout;` would
    // be dropped as "already registered" — leaving the symbol unexported.
    auto isExternRef = [](const wvmcc::parser::DeclarationPtr& d) {
        return d && d->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern);
    };
    for (int pass = 0; pass < 2; ++pass) {
        const bool wantExtern = (pass == 1);
        for (const auto& external : tu->externals) {
            if (!external) continue;
            if (auto d = std::get_if<wvmcc::parser::DeclarationPtr>(&external->decl)) {
                if (*d && isExternRef(*d) == wantExtern) registerGlobalVar(*d);
            }
        }
    }
}

void ModuleCodegen::registerGlobalVar(const wvmcc::parser::DeclarationPtr& decl) {
    if (!decl || !decl->declarator) return; // type-only decl (e.g. struct def)
    // Function declarators are prototypes — handled by registerFunctionDecl.
    if (decl->declarator->kind == wvmcc::parser::Declarator::Kind::Function) return;

    std::string name = getFuncName(decl->declarator);
    if (name.empty()) return;

    // `typedef` introduces no object.
    if (decl->specifiers.hasStorage(wvmcc::parser::StorageClass::Typedef)) return;

    // `extern` (or a bare prototype) declares a reference, not a definition — no
    // storage in this TU. In Linkable mode we model it the same way `extern`
    // functions and the env system globals are modelled: import a Wasm global
    // `(import "env" <name> (global i64))` that carries the variable's address,
    // and emit `global.get` at each use. The linker resolves that import to the
    // defining TU's exported address-global by name. In Freestanding mode there
    // is no other TU to resolve against, so a reference is genuinely undeclared
    // (codegen's diagnostic handles it).
    if (decl->specifiers.hasStorage(wvmcc::parser::StorageClass::Extern)) {
        if (compileMode_ != CompileMode::Linkable) return;
        if (symbolTable_.lookup(name).has_value()) return;
        auto externType = semantic_.canonicalTypeRepr(decl->specifiers, decl->declarator);
        if (!externType) return;

        WasmVM::WasmImport imp;
        imp.module = "env";
        imp.name = name;
        imp.desc = WasmVM::GlobalType{WasmVM::GlobalType::constant, WasmVM::ValueType::i64};
        WasmVM::index_t gidx = importedGlobalCount(); // index before pushing
        module_.imports.push_back(imp);

        GlobalMem gm;
        gm.type = externType;
        gm.dataSegmentIndex = -1;
        gm.address = 0;
        gm.isImport = true;
        gm.name = name;
        gm.importGlobalIndex = (int)gidx;
        symbolTable_.define(name, gm);
        return;
    }

    std::shared_ptr<wvmcc::parser::TypeNode> typeNode;
    size_t addr = 0;
    size_t size = 0;

    if (auto existing = symbolTable_.lookup(name); existing.has_value()) {
        // Already registered (a runtime global, an earlier tentative definition,
        // or an earlier initialized one). Per 6.9.2 the tentative definitions
        // share the storage of the single initialized definition; so the only
        // declaration with work left to do is one that carries an initializer —
        // apply it to the existing storage. Repeated tentatives are no-ops.
        if (!decl->initializer || !*decl->initializer) return;
        auto* gm = std::get_if<GlobalMem>(&*existing);
        if (!gm || gm->isImport) return; // imported extern: no local storage
        typeNode = gm->type;
        addr = gm->address;
        size = typeMap_.byteSize(typeNode);
        if (size == 0) size = 4;
    } else {
        typeNode = semantic_.canonicalTypeRepr(decl->specifiers, decl->declarator);
        if (!typeNode) return;

        size = typeMap_.byteSize(typeNode);
        size_t align = typeMap_.byteAlignment(typeNode);
        if (size == 0) size = 4;
        if (align == 0) align = 4;

        addr = dataAllocator_.allocate(size, align);

        GlobalMem gm;
        gm.type = typeNode;
        gm.dataSegmentIndex = -1;
        gm.address = addr;
        gm.name = name;
        symbolTable_.define(name, gm);

        // Export this definition's address as a Wasm global so other TUs'
        // `extern` references resolve to it at link time. Materialized after
        // firstPass (once all global imports are counted) so the index is stable.
        if (compileMode_ == CompileMode::Linkable) {
            exportedDataGlobals_.push_back({name, addr});
        }
    }

    // Initializer: C requires a constant expression at file scope. Encode
    // scalar and aggregate (`{...}`) constants into an active data segment;
    // zero-init (no initializer or `= 0`) needs no segment since linear memory
    // starts zeroed. A non-constant initializer leaves storage zeroed.
    if (!decl->initializer || !*decl->initializer) return;
    const auto& init = *decl->initializer;

    std::vector<std::byte> bytes(size, std::byte{0});
    pendingAddrSites_.clear();
    if (!encodeConstInit(typeNode, init, 0, bytes)) { pendingAddrSites_.clear(); return; }
    // Skip an all-zero segment (memory is already zeroed).
    bool allZero = true;
    for (auto b : bytes) if (b != std::byte{0}) { allZero = false; break; }
    if (allZero) { pendingAddrSites_.clear(); return; }

    WasmVM::WasmData seg;
    seg.mode.type = WasmVM::WasmData::DataMode::Mode::active;
    seg.mode.memidx = 0;
    seg.mode.offset = WasmVM::Instr::I64_const{(WasmVM::i64_t)addr};
    seg.init = std::move(bytes);
    size_t dataIdx = module_.datas.size();
    module_.datas.push_back(std::move(seg));

    // LANG-6.6-06: register the address-constant sites encoded into this
    // segment so the linker rebases them. byteOffset is relative to base=0,
    // which is the segment's start (seg.init == the object's bytes).
    for (const auto& [boff, isFunc] : pendingAddrSites_) {
        if (isFunc) dataSegFuncPtrRelocs_.push_back({dataIdx, boff});
        else        dataSegDataRelocs_.push_back({dataIdx, boff});
    }
    pendingAddrSites_.clear();
}

void ModuleCodegen::emitStringLiterals() {
    for (auto& seg : dataAllocator_.getDataSegments()) {
        module_.datas.push_back(std::move(seg));
    }
}

// ---------------------------------------------------------------------------
// Hosted-environment support (issue #40)
// ---------------------------------------------------------------------------

void ModuleCodegen::scanForMain(const wvmcc::parser::TranslationUnitPtr& tu) {
    if (!tu) return;
    for (const auto& ext : tu->externals) {
        if (!ext) continue;
        auto fd = std::get_if<wvmcc::parser::FunctionDefPtr>(&ext->decl);
        if (!fd || !*fd) continue;
        std::string name = getFuncName((*fd)->declarator);
        if (name != "main") continue;
        hasMain_ = true;
        // Determine if main takes the (argc, argv) form by looking at the
        // function declarator's parameter list.
        if ((*fd)->declarator) {
            auto cur = (*fd)->declarator;
            while (cur && cur->kind != wvmcc::parser::Declarator::Kind::Function) {
                cur = cur->inner.has_value() ? *cur->inner : nullptr;
            }
            if (cur && cur->function.params.size() == 2) {
                mainHasArgv_ = true;
            }
        }
        return;
    }
}

void ModuleCodegen::injectSysProcImports() {
    auto imports = startwrapper::injectSysProcImports(module_, nextFuncIndex_);
    sysProcArgcIdx_    = (int)imports.argc;
    sysProcArgvLenIdx_ = (int)imports.argvLen;
    sysProcArgvIdx_    = (int)imports.argv;
    sysProcExitIdx_    = (int)imports.exit;
}

void ModuleCodegen::emitStartWrapper() {
    if (mainFuncIndex_ < 0) return;
    startwrapper::SysProcImports imports{
        (WasmVM::index_t)sysProcArgcIdx_,
        (WasmVM::index_t)sysProcArgvLenIdx_,
        (WasmVM::index_t)sysProcArgvIdx_,
        (WasmVM::index_t)sysProcExitIdx_,
    };
    startwrapper::emitStartWrapper(module_,
                                   imports,
                                   (WasmVM::index_t)mainFuncIndex_,
                                   mainHasArgv_);
}

} // namespace wvmcc::codegen
