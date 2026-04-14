#include "SymbolTable.hpp"

namespace wvmcc::codegen {

void SymbolTable::pushScope() {
    scopes_.emplace_back();
    funcScopes_.emplace_back();
}

void SymbolTable::popScope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
        funcScopes_.pop_back();
    }
}

bool SymbolTable::define(const std::string& name, const VarInfoStruct& info) {
    if (scopes_.empty()) {
        return false;
    }
    scopes_.back()[name] = info;
    return true;
}

bool SymbolTable::defineFunction(const std::string& name, const FuncSymbol& func) {
    if (funcScopes_.empty()) {
        return false;
    }
    funcScopes_.back()[name] = func;
    return true;
}

std::optional<VarInfoStruct> SymbolTable::lookup(const std::string& name) const {
    // Search from innermost scope outward
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

std::optional<FuncSymbol> SymbolTable::lookupFunction(const std::string& name) const {
    // Search from innermost scope outward
    for (auto it = funcScopes_.rbegin(); it != funcScopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

bool SymbolTable::exists(const std::string& name) const {
    return lookup(name).has_value();
}

} // namespace wvmcc::codegen