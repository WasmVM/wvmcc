#ifndef WASMVM_PP_ErrorListener
#define WASMVM_PP_ErrorListener

#include <antlr4-runtime.h>

namespace WasmVM {

struct PPParseErrorListener : antlr4::BaseErrorListener {
    void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token * offendingSymbol, size_t line, size_t col, const std::string &msg, std::exception_ptr e) override;
};

}

#endif