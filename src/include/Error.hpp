#ifndef WVMCC_Error_DEF
#define WVMCC_Error_DEF

#include <exception>
#include <string>
#include <common.hpp>

namespace WasmVM {
namespace Exception {

struct Error : public std::runtime_error {
    Error(SourcePos pos, std::string msg) : std::runtime_error(msg), pos(pos){}
    SourcePos pos;
};

struct SyntaxError : public Error {
    SyntaxError(SourcePos pos, std::string msg) : Error(pos, "syntax error: " + msg){}
};

} // namespace Exception
} // namespace WasmVM

#endif