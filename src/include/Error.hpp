#ifndef WVMCC_Error_DEF
#define WVMCC_Error_DEF

#include <exception.hpp>
#include <SourceStream.hpp>

namespace WasmVM {
namespace Exception {

struct Error : public Exception {
    Error(SourcePos pos, std::string msg) : Exception(msg), pos(pos){}
    SourcePos pos;
};

struct SyntaxError : public Error {
    SyntaxError(SourcePos pos, std::string msg) : Error(pos, "syntax error:" + msg){}
};

} // namespace WasmVM
} // namespace WasmVM

#endif