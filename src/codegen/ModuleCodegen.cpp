#include "ModuleCodegen.hpp"
#include "FunctionCodegen.hpp"
#include <stdexcept>

namespace wvmcc::codegen {

ModuleCodegen::ModuleCodegen(const wvmcc::parser::Semantic& semantic)
    : semantic_(semantic) {}

WasmVM::WasmModule ModuleCodegen::generate(const wvmcc::parser::TranslationUnitPtr& tu) {
    module_ = WasmVM::WasmModule{};
    nextFuncIndex_ = 0;

    setupMemory();
    setupGlobals();

    symbolTable_.pushScope();
    firstPass(tu);
    secondPass(tu);
    symbolTable_.popScope();

    return module_;
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

WasmVM::FuncType ModuleCodegen::buildFuncTypeFromDef(const wvmcc::parser::FunctionDefPtr& funcDef) const {
    WasmVM::FuncType ft;

    bool isVoid = false;
    for (const auto& ts : funcDef->specifiers.typeSpecifiers) {
        if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
            && !ts.simple.empty()
            && ts.simple[0] == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void) {
            isVoid = true;
        }
        break;
    }
    if (!isVoid) {
        ft.results.push_back(WasmVM::ValueType::i32);
    }

    for (size_t i = 0; i < funcDef->params.size(); ++i) {
        ft.params.push_back(WasmVM::ValueType::i32);
    }

    return ft;
}

WasmVM::FuncType ModuleCodegen::buildFuncTypeFromDecl(const wvmcc::parser::DeclarationPtr& decl) const {
    WasmVM::FuncType ft;

    bool isVoid = false;
    for (const auto& ts : decl->specifiers.typeSpecifiers) {
        if (ts.kind == wvmcc::parser::DeclarationSpecifiers::TypeSpecifier::Kind::Simple
            && !ts.simple.empty()
            && ts.simple[0] == wvmcc::parser::DeclarationSpecifiers::SimpleTypeSpecifier::Void) {
            isVoid = true;
        }
        break;
    }
    if (!isVoid) {
        ft.results.push_back(WasmVM::ValueType::i32);
    }

    if (decl->declarator && decl->declarator->kind == wvmcc::parser::Declarator::Kind::Function) {
        for (size_t i = 0; i < decl->declarator->function.params.size(); ++i) {
            ft.params.push_back(WasmVM::ValueType::i32);
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
    internFuncType(ft);

    std::string name = getFuncName(funcDef->declarator);
    FuncSymbol sym;
    sym.type = nullptr;
    sym.funcIndex = nextFuncIndex_++;
    sym.isImport = false;
    symbolTable_.defineFunction(name, sym);
}

void ModuleCodegen::registerFunctionDeclaration(const wvmcc::parser::DeclarationPtr& decl) {
    if (!decl || !decl->declarator) return;

    auto ft = buildFuncTypeFromDecl(decl);
    WasmVM::index_t typeidx = internFuncType(ft);

    std::string name = getFuncName(decl->declarator);

    WasmVM::WasmImport imp;
    imp.module = "env";
    imp.name = name;
    imp.desc = typeidx;
    module_.imports.push_back(imp);

    FuncSymbol sym;
    sym.type = nullptr;
    sym.funcIndex = nextFuncIndex_++;
    sym.isImport = true;
    symbolTable_.defineFunction(name, sym);
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
}

void ModuleCodegen::emitFunctionDefinition(const wvmcc::parser::FunctionDefPtr& funcDef) {
    if (!funcDef) return;

    FunctionCodegen funcCodegen(typeMap_, symbolTable_, &dataAllocator_);
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
}

} // namespace wvmcc::codegen
