#include <iostream>
#include <fstream>
#include <string>

#include <antlr4-runtime.h>
#include <exception.hpp>
#include <PPLexer.h>
#include <PPParser.h>
#include "ErrorListener.hpp"

int main(int argc, char const *argv[])
{

    // Warnings
    WasmVM::Exception::Warning::regist([](std::string message){
        std::cerr << "Warning : " << message << std::endl;
    });

    std::ifstream fin("test.txt");
    antlr4::ANTLRInputStream input(fin);
    WasmVM::PPLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    WasmVM::PPParser parser(&tokens);

    WasmVM::PPParseErrorListener errorListener;

    parser.removeErrorListeners();
    parser.addErrorListener(&errorListener);

    while (parser.isMatchedEOF() == false){
        antlr4::tree::ParseTree* group = parser.group();
        std::cout << group->toStringTree(&parser) << std::endl;
    }
    return 0;
}