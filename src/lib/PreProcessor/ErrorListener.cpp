// Copyright 2025 Luis Hsu. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "ErrorListener.hpp"

#include <Error.hpp>
#include <PPParser.h>

void WasmVM::PPParseErrorListener::syntaxError(
    antlr4::Recognizer* recognizer,
    antlr4::Token* offendingSymbol,
    size_t line, size_t col,
    const std::string &msg, std::exception_ptr
){
    // Warning : not end with new_line
    WasmVM::PPParser* parser = static_cast<WasmVM::PPParser*>(recognizer);
    if((offendingSymbol->getType() == PPParser::EOF) && (parser->getRuleInvocationStack().front() == "text_line")){
        Exception::Warning("source file should end with new-line");
    }
}