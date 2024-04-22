#ifndef WVMCC_TransUnit_DEF
#define WVMCC_TransUnit_DEF

#include <vector>
#include <variant>
#include <AST/Decl.hpp>

namespace WasmVM {

using ExternDecl = std::variant<Decl>;

struct TransUnit {
    std::vector<ExternDecl> decls;
};

};

#endif