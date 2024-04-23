#ifndef WVMCC_AST_Dump_DEF
#define WVMCC_AST_Dump_DEF

#include <ostream>
#include <AST/Expr.hpp>

std::ostream& operator<< (std::ostream&, WasmVM::AST::PrimaryExpr&&);

#endif