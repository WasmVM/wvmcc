#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "Tokenizer.hpp"

namespace wvmcc {

struct Macro {
    std::string name;
    bool isFunction{false};           // true if function-like, false if object-like
    std::vector<std::string> params;  // parameter names for function-like macros
    std::vector<PPToken> replacement; // replacement token stream
    bool variadic{false};             // true if macro has ... (variadic)
};

class MacroTable {
public:
    // Define an object-like macro: #define NAME replacement
    void defineObjectMacro(const std::string& name,
                          const std::vector<PPToken>& replacement);

    // Define a function-like macro: #define NAME(params...) replacement
    void defineFunctionMacro(const std::string& name,
                            const std::vector<std::string>& params,
                            const std::vector<PPToken>& replacement,
                            bool variadic = false);

    // Undefine a macro
    void undefine(const std::string& name);

    // Check if a macro is defined
    bool isDefined(const std::string& name) const;

    // Get macro definition; returns nullopt if not defined
    std::optional<const Macro*> getMacro(const std::string& name) const;

    // Clear all macros
    void clear();

    // Get number of defined macros (for testing)
    size_t count() const { return macros.size(); }

private:
    std::unordered_map<std::string, Macro> macros;
};

} // namespace wvmcc
