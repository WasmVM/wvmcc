#include "ModuleCodegen.hpp"
#include "FunctionCodegen.hpp"
#include "AddressTakenAnalyzer.hpp"
#include <stdexcept>

namespace wvmcc::codegen {

ModuleCodegen::ModuleCodegen(const wvmcc::parser::Semantic& semantic)
    : semantic_(semantic) {}

WasmVM::WasmModule ModuleCodegen::generate(const wvmcc::parser::TranslationUnitPtr& tu) {
    module_ = WasmVM::WasmModule{};
    nextFuncIndex_ = 0;
    funcTypeIdx_.clear();
    funcTableSlots_.clear();
    hasMain_ = false;
    mainHasArgv_ = false;
    mainFuncIndex_ = -1;
    sysProcArgcIdx_ = sysProcArgvLenIdx_ = sysProcArgvIdx_ = sysProcExitIdx_ = -1;

    setupMemory();
    setupGlobals();

    // Detect main() and inject sys_proc imports before any user functions are
    // registered, so the sys_proc imports occupy function indices 0..3.
    scanForMain(tu);
    if (hasMain_) {
        injectSysProcImports();
    }

    symbolTable_.pushScope();
    firstPass(tu);
    analyzeFuncAddressTaken(tu);
    secondPass(tu);
    symbolTable_.popScope();

    if (hasMain_) {
        emitStartWrapper();
    }

    return module_;
}

std::optional<size_t> ModuleCodegen::getFuncTableSlot(const std::string& name) const {
    auto it = funcTableSlots_.find(name);
    if (it == funcTableSlots_.end()) return std::nullopt;
    return it->second;
}

std::optional<WasmVM::index_t> ModuleCodegen::getFuncTypeIdx(const std::string& name) const {
    auto it = funcTypeIdx_.find(name);
    if (it == funcTypeIdx_.end()) return std::nullopt;
    return it->second;
}

WasmVM::index_t ModuleCodegen::allocateGuardGlobal() {
    WasmVM::WasmGlobal g;
    g.type = WasmVM::GlobalType{WasmVM::GlobalType::variable, WasmVM::ValueType::i32};
    g.init = WasmVM::Instr::I32_const{0};
    module_.globals.push_back(g);
    return (WasmVM::index_t)(module_.globals.size() - 1);
}

size_t ModuleCodegen::allocateStaticStorage(size_t size, size_t align) {
    return dataAllocator_.allocate(size, align);
}

void ModuleCodegen::setupMemory() {
    WasmVM::MemType mem0;
    mem0.min = 1;
    mem0.is64 = true;
    mem0.max = std::nullopt;

    WasmVM::MemType mem1;
    mem1.min = 1;
    mem1.is64 = true;
    mem1.max = std::nullopt;

    module_.mems.push_back(mem0);
    module_.mems.push_back(mem1);
}

void ModuleCodegen::setupGlobals() {
    WasmVM::WasmGlobal stackPointerGlobal;
    stackPointerGlobal.type = WasmVM::GlobalType{WasmVM::GlobalType::variable, WasmVM::ValueType::i64};
    stackPointerGlobal.init = WasmVM::Instr::I64_const{0x10000};

    module_.globals.push_back(stackPointerGlobal);
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

    auto baseType = semantic.buildTypeFromDeclaration(specs, nullptr);

    // Walk the declarator chain: collect Pointer/Array nodes that sit BEFORE the
    // Function (or Identifier) node.  These qualify the return type.
    std::vector<wvmcc::parser::Declarator::Kind> quals;
    for (auto cur = decl; cur;
         cur = (cur->inner.has_value() ? *cur->inner : nullptr)) {
        if (cur->kind == wvmcc::parser::Declarator::Kind::Function
            || cur->kind == wvmcc::parser::Declarator::Kind::Identifier) break;
        quals.push_back(cur->kind);
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
            auto paramType = semantic_.buildTypeFromDeclaration(param.specifiers, param.declarator);
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
                auto paramType = semantic_.buildTypeFromDeclaration(param.specifiers, param.declarator);
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
    for (const auto& external : tu->externals) {
        registerExternalDecl(external);
    }
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
    sym.namedParamCount = isVoidParamList(funcDef->params)
                              ? 0
                              : static_cast<int>(funcDef->params.size());
    symbolTable_.defineFunction(name, sym);

    if (name == "main") {
        mainFuncIndex_ = sym.funcIndex;
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
    sym.type = nullptr;
    sym.funcIndex = nextFuncIndex_++;
    sym.isImport = true;
    if (decl->declarator->kind == wvmcc::parser::Declarator::Kind::Function) {
        sym.isVariadic = decl->declarator->function.isVariadic;
        const auto& dparams = decl->declarator->function.params;
        sym.namedParamCount = isVoidParamList(dparams)
                                  ? 0
                                  : static_cast<int>(dparams.size());
    }
    symbolTable_.defineFunction(name, sym);
}

// ---------------------------------------------------------------------------
// Function-pointer support: scan every function body for `&funcname` usage,
// allocate one funcref-table slot per address-taken function, and emit an
// active element segment populating the table.
// ---------------------------------------------------------------------------

void ModuleCodegen::analyzeFuncAddressTaken(const wvmcc::parser::TranslationUnitPtr& tu) {
    if (!tu) return;

    AddressTakenAnalyzer analyzer;
    std::unordered_set<std::string> allTaken;
    for (const auto& ext : tu->externals) {
        if (!ext) continue;
        if (auto fd = std::get_if<wvmcc::parser::FunctionDefPtr>(&ext->decl)) {
            if (!*fd) continue;
            auto names = analyzer.analyze(*fd);
            for (auto& n : names) allTaken.insert(n);
        }
    }

    // Filter to names that resolve to a function symbol.
    std::vector<std::pair<std::string, int>> tableFuncs;  // name → funcIndex
    for (const auto& name : allTaken) {
        auto sym = symbolTable_.lookupFunction(name);
        if (!sym) continue;
        size_t slot = tableFuncs.size();
        funcTableSlots_[name] = slot;
        tableFuncs.emplace_back(name, sym->funcIndex);
    }

    if (tableFuncs.empty()) return;

    // Add a single funcref table sized exactly to fit all slots.
    WasmVM::TableType t;
    t.limits.min = (WasmVM::offset_t)tableFuncs.size();
    t.limits.max = (WasmVM::offset_t)tableFuncs.size();
    t.limits.is64 = false;
    t.reftype = WasmVM::RefType::funcref;
    module_.tables.push_back(t);

    // Active element segment: populate the table at offset 0.
    WasmVM::WasmElem elem;
    elem.type = WasmVM::RefType::funcref;
    elem.mode.type = WasmVM::WasmElem::ElemMode::Mode::active;
    elem.mode.tableidx = 0;
    elem.mode.offset = WasmVM::Instr::I32_const{0};
    for (const auto& [name, funcIdx] : tableFuncs) {
        (void)name;
        elem.elemlist.push_back(WasmVM::Instr::Ref_func{(WasmVM::index_t)funcIdx});
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

    auto ft = buildFuncTypeFromDef(funcDef);
    wasmFunc.typeidx = internFuncType(ft);

    module_.funcs.push_back(wasmFunc);
}

void ModuleCodegen::emitGlobalScalar(const wvmcc::parser::DeclarationPtr& decl) {
    WasmVM::WasmGlobal global;
    global.type = WasmVM::GlobalType{WasmVM::GlobalType::variable, WasmVM::ValueType::i64};
    global.init = WasmVM::Instr::I64_const{0};
    module_.globals.push_back(global);
}

void ModuleCodegen::emitGlobalAggregate(const wvmcc::parser::DeclarationPtr& decl) {
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
    auto registerImport = [&](const std::string& name,
                              std::vector<WasmVM::ValueType> params,
                              std::vector<WasmVM::ValueType> results) -> int {
        WasmVM::FuncType ft;
        ft.params = std::move(params);
        ft.results = std::move(results);
        WasmVM::index_t typeidx = internFuncType(ft);
        WasmVM::WasmImport imp;
        imp.module = "sys_proc";
        imp.name = name;
        imp.desc = typeidx;
        module_.imports.push_back(imp);
        int idx = nextFuncIndex_++;
        return idx;
    };

    sysProcArgcIdx_    = registerImport("argc",     {},                                                   {WasmVM::ValueType::i32});
    sysProcArgvLenIdx_ = registerImport("argv_len", {WasmVM::ValueType::i32},                             {WasmVM::ValueType::i32});
    sysProcArgvIdx_    = registerImport("argv",     {WasmVM::ValueType::i32, WasmVM::ValueType::i64,
                                                     WasmVM::ValueType::i64},                            {WasmVM::ValueType::i32});
    sysProcExitIdx_    = registerImport("exit",     {WasmVM::ValueType::i32},                             {});
}

void ModuleCodegen::emitStartWrapper() {
    if (mainFuncIndex_ < 0) return;

    // FuncType: () -> ()
    WasmVM::FuncType ft;
    WasmVM::index_t wrapperType = internFuncType(ft);

    // Stack pointer global is module_.globals[0] (see setupGlobals()).
    constexpr WasmVM::index_t kSpGlobal = 0;

    WasmVM::WasmFunc wrapper;
    wrapper.typeidx = wrapperType;
    auto& body = wrapper.body;

    if (mainHasArgv_) {
        // Locals: [argc:i32, i:i32, len:i32, argv_base:i64, sp_save:i64]
        wrapper.locals = {
            WasmVM::ValueType::i32, // 0: argc
            WasmVM::ValueType::i32, // 1: i
            WasmVM::ValueType::i32, // 2: len
            WasmVM::ValueType::i64, // 3: argv_base
            WasmVM::ValueType::i64, // 4: sp_save
        };

        // argc = sys_proc.argc()
        body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)sysProcArgcIdx_});
        body.push_back(WasmVM::Instr::Local_set{0});
        // sp_save = sp
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::Local_set{4});
        // argv_base = sp - argc * 8;  sp = argv_base
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::Local_get{0});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::I64_const{8});
        body.push_back(WasmVM::Instr::I64_mul{});
        body.push_back(WasmVM::Instr::I64_sub{});
        body.push_back(WasmVM::Instr::Local_tee{3});
        body.push_back(WasmVM::Instr::Global_set{kSpGlobal});
        // i = 0
        body.push_back(WasmVM::Instr::I32_const{0});
        body.push_back(WasmVM::Instr::Local_set{1});
        // Block / Loop
        body.push_back(WasmVM::Instr::Block{std::nullopt});
        body.push_back(WasmVM::Instr::Loop{std::nullopt});
        // if (i >= argc) br outer (label index 1 from inside Loop)
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::Local_get{0});
        body.push_back(WasmVM::Instr::I32_ge_s{});
        body.push_back(WasmVM::Instr::Br_if{1});
        // len = sys_proc.argv_len(i)
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)sysProcArgvLenIdx_});
        body.push_back(WasmVM::Instr::Local_set{2});
        // sp -= (len + 8) & ~7
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::Local_get{2});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::I64_const{8});
        body.push_back(WasmVM::Instr::I64_add{});
        body.push_back(WasmVM::Instr::I64_const{-8});
        body.push_back(WasmVM::Instr::I64_and{});
        body.push_back(WasmVM::Instr::I64_sub{});
        body.push_back(WasmVM::Instr::Global_set{kSpGlobal});
        // argv_base[i] = sp  (store i64 pointer, mem 0)
        body.push_back(WasmVM::Instr::Local_get{3});
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::I64_const{8});
        body.push_back(WasmVM::Instr::I64_mul{});
        body.push_back(WasmVM::Instr::I64_add{});
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        // 8-byte aligned: argv_base is computed from an 8-aligned stack pointer
        // and indexed by i*8 (align hint log2(8) = 3).
        body.push_back(WasmVM::Instr::I64_store{0, 0, 3});
        // sys_proc.argv(i, sp, (i64)(len + 1))
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::Global_get{kSpGlobal});
        body.push_back(WasmVM::Instr::Local_get{2});
        body.push_back(WasmVM::Instr::I32_const{1});
        body.push_back(WasmVM::Instr::I32_add{});
        body.push_back(WasmVM::Instr::I64_extend_i32_s{});
        body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)sysProcArgvIdx_});
        body.push_back(WasmVM::Instr::Drop{});
        // ++i
        body.push_back(WasmVM::Instr::Local_get{1});
        body.push_back(WasmVM::Instr::I32_const{1});
        body.push_back(WasmVM::Instr::I32_add{});
        body.push_back(WasmVM::Instr::Local_set{1});
        // br loop (label 0)
        body.push_back(WasmVM::Instr::Br{0});
        body.push_back(WasmVM::Instr::End{}); // end loop
        body.push_back(WasmVM::Instr::End{}); // end block
        // call main(argc, argv_base) -> i32, then sys_proc.exit
        body.push_back(WasmVM::Instr::Local_get{0});
        body.push_back(WasmVM::Instr::Local_get{3});
        body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)mainFuncIndex_});
        body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)sysProcExitIdx_});
        body.push_back(WasmVM::Instr::Unreachable{});
    } else {
        // main(void) wrapper: call main; pass result to exit
        body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)mainFuncIndex_});
        body.push_back(WasmVM::Instr::Call{(WasmVM::index_t)sysProcExitIdx_});
        body.push_back(WasmVM::Instr::Unreachable{});
    }
    body.push_back(WasmVM::Instr::End{});

    WasmVM::index_t wrapperIdx =
        (WasmVM::index_t)(module_.imports.size() + module_.funcs.size());
    module_.funcs.push_back(std::move(wrapper));
    module_.start = wrapperIdx;

    WasmVM::WasmExport ex;
    ex.name = "main";
    ex.desc = WasmVM::WasmExport::DescType::func;
    ex.index = (WasmVM::index_t)mainFuncIndex_;
    module_.exports.push_back(ex);
}

} // namespace wvmcc::codegen
