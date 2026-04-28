// Unit test: declarator parsing (pointers, arrays, functions, parameter arrays)
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

int main() {
    const std::string fname = "temp_declarator_test.c";
    {
        std::ofstream ofs(fname);
        ofs << "int *p;\n";
        ofs << "int arr[10];\n";
        ofs << "int f(int a) { return 0; }\n";
        ofs << "void g(int a[static 5]) { (void)a; }\n";
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    if (!tu) { std::remove(fname.c_str()); return 3; }

    

    // Expect four externals
    if (tu->externals.size() != 4) { std::remove(fname.c_str()); return 4; }

    // 0: int *p;
    {
        auto &ext = tu->externals[0];
        if (!std::holds_alternative<DeclarationPtr>(ext->decl)) return 5;
        auto d = std::get<DeclarationPtr>(ext->decl);
        if (!d->declarator) return 6;
        // top-level declarator is identifier 'p'
        if (d->declarator->kind != Declarator::Kind::Identifier) return 7;
        if (!d->declarator->inner.has_value()) return 8;
        auto inner = *d->declarator->inner;
        if (!inner) return 9;
        if (inner->kind != Declarator::Kind::Pointer) return 10;
    }

    // 1: int arr[10]; -- parser returns outermost Array node
    {
        auto &ext = tu->externals[1];
        if (!std::holds_alternative<DeclarationPtr>(ext->decl)) return 11;
        auto d = std::get<DeclarationPtr>(ext->decl);
        if (!d->declarator) return 12;
        // top-level should be Array (array is outermost)
        if (d->declarator->kind != Declarator::Kind::Array) return 13;
        auto arrnode = d->declarator;
        if (!arrnode->inner.has_value()) return 14;
        auto innerId = *arrnode->inner;
        if (!innerId) return 15;
        if (innerId->kind != Declarator::Kind::Identifier) return 16;
        if (innerId->id.name != "arr") return 17;
        if (!arrnode->array.size.has_value()) return 18;
        // expect integer literal 10
        auto sz = *(arrnode->array.size);
        auto il = std::dynamic_pointer_cast<IntegerLiteral>(sz);
        if (!il) return 19;
        if (il->value != 10) return 20;
    }

    // 2: int f(int a) { ... }
    {
        auto &ext = tu->externals[2];
        if (!std::holds_alternative<FunctionDefPtr>(ext->decl)) return 20;
        auto f = std::get<FunctionDefPtr>(ext->decl);
        if (!f->declarator) return 21;
        // function declarator should appear in declarator chain
        auto cur = f->declarator;
        bool foundFn = false;
        while (cur) {
            if (cur->kind == Declarator::Kind::Function) { foundFn = true; break; }
            cur = cur->inner.has_value() ? *cur->inner : nullptr;
        }
        if (!foundFn) return 22;
        // verify parameter name
        // the function declarator holds parameter list in its FunctionInfo
        // locate the Function node
        cur = f->declarator;
        while (cur && cur->kind != Declarator::Kind::Function) cur = cur->inner.has_value() ? *cur->inner : nullptr;
        if (!cur) return 23;
        if (!cur->function.hasParamTypeList) return 24;
        if (cur->function.params.size() != 1) return 25;
        auto &param = cur->function.params[0];
        if (!param.declarator) return 26;
        if (param.declarator->id.name != "a") return 27;
    }

    // 3: void g(int a[static 5]) { ... }
    {
        auto &ext = tu->externals[3];
        if (!std::holds_alternative<FunctionDefPtr>(ext->decl)) return 28;
        auto g = std::get<FunctionDefPtr>(ext->decl);
        if (!g->declarator) return 29;
        auto cur = g->declarator;
        while (cur && cur->kind != Declarator::Kind::Function) cur = cur->inner.has_value() ? *cur->inner : nullptr;
        if (!cur) return 30;
        if (!cur->function.hasParamTypeList) return 31;
        if (cur->function.params.size() != 1) return 32;
        auto &param = cur->function.params[0];
        if (!param.declarator) return 33;
        // parameter declarator for 'a[static 5]' will be an Array node (outermost)
        if (param.declarator->kind != Declarator::Kind::Array) return 34;
        auto arrnode = param.declarator;
        if (!arrnode->array.isStatic) return 35;
        if (!arrnode->array.size.has_value()) return 36;
        auto sz = *(arrnode->array.size);
        auto il = std::dynamic_pointer_cast<IntegerLiteral>(sz);
        if (!il) return 37;
        if (il->value != 5) return 38;
    }

    std::remove(fname.c_str());
    std::cout << "declarator-test: OK" << std::endl;
    return 0;
}
