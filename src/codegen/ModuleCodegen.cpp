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
        for (const auto& param : decl->declarator->function.params) {
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
    sym.type = buildReturnTypeNode(funcDef->specifiers, funcDef->declarator, semantic_);
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
    emitStringLiterals();
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
    for (auto& seg : dataAllocator_.getDataSegments()) {
        module_.datas.push_back(std::move(seg));
    }
}

} // namespace wvmcc::codegen
