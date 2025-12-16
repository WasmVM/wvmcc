#include "MacroTable.hpp"

namespace wvmcc {

void MacroTable::defineObjectMacro(const std::string& name,
                                   const std::vector<PPToken>& replacement) {
    macros[name] = Macro{
        .name = name,
        .isFunction = false,
        .params = {},
        .replacement = replacement,
        .variadic = false,
    };
}

void MacroTable::defineFunctionMacro(const std::string& name,
                                     const std::vector<std::string>& params,
                                     const std::vector<PPToken>& replacement,
                                     bool variadic) {
    macros[name] = Macro{
        .name = name,
        .isFunction = true,
        .params = params,
        .replacement = replacement,
        .variadic = variadic,
    };
}

void MacroTable::undefine(const std::string& name) {
    macros.erase(name);
}

bool MacroTable::isDefined(const std::string& name) const {
    return macros.find(name) != macros.end();
}

std::optional<const Macro*> MacroTable::getMacro(const std::string& name) const {
    auto it = macros.find(name);
    if (it != macros.end()) {
        return &it->second;
    }
    return std::nullopt;
}

void MacroTable::clear() {
    macros.clear();
}

} // namespace wvmcc
