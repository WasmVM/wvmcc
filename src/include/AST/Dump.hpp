#ifndef WVMCC_AST_Dump_DEF
#define WVMCC_AST_Dump_DEF

#include <ostream>
#include <AST/Expr.hpp>

namespace WasmVM {
namespace AST {

std::ostream& operator<< (std::ostream&, const PrimaryExpr&);

} // namespace AST
} // namespace WasmVM
#endif