#include <iostream>
#include <string>
#include <fstream>

#include <exception.hpp>
#include <Error.hpp>
#include <PreProcessor.hpp>

using namespace WasmVM;

std::ostream& operator<< (std::ostream& os, const PreProcessor::Token& token){
    os << token.pos.file.filename().string() << ":" << token.pos.line << ":" << token.pos.col << " ";
    switch (token.type){
        case PreProcessor::Token::HeaderName:
            os << "HeaderName: " << token.text;
            break;
        case PreProcessor::Token::Identifier:
            os << "Identifier: " << token.text;
            break;
        case PreProcessor::Token::PPNumber:
            os << "PPNumber: " << token.text;
            break;
        case PreProcessor::Token::CharConst:
            os << "CharConst: " << token.text;
            break;
        case PreProcessor::Token::StringLiteral:
            os << "StringLiteral: " << token.text;
            break;
        case PreProcessor::Token::Punctuator:
            os << "Punctuator: " << token.text;
            break;
        case PreProcessor::Token::WhiteSpace:
            os << "WhiteSpace: " << token.text;
            break;
        case PreProcessor::Token::NewLine:
            os << "NewLine";
            break;
    }
    return os;
}

int main(int argc, char const *argv[])
{

    // Warnings
    Exception::Warning::regist([](std::string message){
        std::cerr << "Warning : " << message << std::endl;
    });

    PreProcessor pp("test.c");

    try{
        for(std::optional<PreProcessor::Token> token = pp.get(); token.has_value(); token = pp.get()){
            std::cout << token.value() << std::endl;
        }
    }catch(Exception::Error &e){
        std::cerr << e.pos.file.filename() << ":" << e.pos.line << ":" << e.pos.col << ": error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}