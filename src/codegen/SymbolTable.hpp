#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <variant>
#include "../parser/AST.hpp"
#include <WasmVM.hpp>

namespace wvmcc::codegen {

// Variable information structures (must be defined before SymbolTable)
struct ScalarLocal {
    wvmcc::parser::TypeNodePtr type;
    bool isAddressTaken;
    int localIndex; // Index in Wasm function locals
};

struct MemoryLocal {
    wvmcc::parser::TypeNodePtr type;
    size_t frameOffset; // Offset from frame pointer
};

struct GlobalScalar {
    wvmcc::parser::TypeNodePtr type;
    bool isMutable;
    int globalIndex; // Index in Wasm module globals
};

struct GlobalMem {
    wvmcc::parser::TypeNodePtr type;
    int dataSegmentIndex; // Index in Wasm module data segments
    size_t address; // Memory address
};

struct FuncSymbol {
    wvmcc::parser::TypeNodePtr type;
    int funcIndex; // Index in Wasm module functions
    bool isImport; // Whether this is an imported function
    bool isVariadic{false};       // trailing `...` in C signature
    int namedParamCount{0};       // number of named (non-variadic) C parameters
    // Wasm value types for each named C parameter. Used at call sites to
    // coerce argument values (e.g. i32 → i64) so passing an integer
    // literal to a function whose parameter is i64 doesn't break
    // validation. Excludes any hidden trailing variadic spill-base param.
    std::vector<WasmVM::ValueType> paramTypes;
};

// Variant type for different symbol kinds
using VarInfoStruct = std::variant<ScalarLocal, MemoryLocal, GlobalScalar, GlobalMem, FuncSymbol>;

class SymbolTable {
public:
    // Scope management
    void pushScope();
    void popScope();

    // Symbol definition and lookup
    bool define(const std::string& name, const VarInfoStruct& info);
    bool defineFunction(const std::string& name, const FuncSymbol& func);
    std::optional<VarInfoStruct> lookup(const std::string& name) const;
    std::optional<FuncSymbol> lookupFunction(const std::string& name) const;

    // Check if a symbol exists at any scope
    bool exists(const std::string& name) const;

private:
    // Scope stack
    std::vector<std::unordered_map<std::string, VarInfoStruct>> scopes_;
    std::vector<std::unordered_map<std::string, FuncSymbol>> funcScopes_;
};

} // namespace wvmcc::codegen
