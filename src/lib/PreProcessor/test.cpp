#include <iostream>
#include <fstream>
#include <string>

#include <antlr4-runtime.h>
#include <PPToken.h>

int main(int argc, char const *argv[])
{
    std::ifstream fin("test.txt");
    antlr4::ANTLRInputStream input(fin);
    WasmVM::PPToken lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    tokens.fill();
    for (auto token : tokens.getTokens()) {
        std::cout << token->getType() << "\t" << token->toString() << std::endl;
    }
    fin.close();
    return 0;
}