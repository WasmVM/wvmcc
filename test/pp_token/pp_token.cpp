// Copyright (C) 2023 Luis Hsu
// 
// wvmcc is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// wvmcc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with wvmcc. If not, see <http://www.gnu.org/licenses/>.

#include <harness.hpp>
#include <PPToken.h>

using namespace WasmVM;
using namespace Testing;

static 

Suite pp_token {
    Test("punctuator", {
        std::ifstream fin("punctuator.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::Punctuator);
        Expect(tokens[2]->getType() == PPToken::Punctuator);
        Expect(tokens[4]->getType() == PPToken::Punctuator);
        Expect(tokens[6]->getType() == PPToken::Punctuator);
        Expect(tokens[8]->getType() == PPToken::Punctuator);
        Expect(tokens[10]->getType() == PPToken::Punctuator);
        Expect(tokens[12]->getType() == PPToken::Punctuator);
        Expect(tokens[14]->getType() == PPToken::Punctuator);
        Expect(tokens[16]->getType() == PPToken::Punctuator);
        Expect(tokens[18]->getType() == PPToken::Punctuator);
        Expect(tokens[20]->getType() == PPToken::Punctuator);
        Expect(tokens[22]->getType() == PPToken::Punctuator);
        Expect(tokens[24]->getType() == PPToken::Punctuator);
        Expect(tokens[26]->getType() == PPToken::Punctuator);
        Expect(tokens[28]->getType() == PPToken::Punctuator);
        Expect(tokens[30]->getType() == PPToken::Punctuator);
        Expect(tokens[32]->getType() == PPToken::Punctuator);
        Expect(tokens[34]->getType() == PPToken::Punctuator);
        Expect(tokens[36]->getType() == PPToken::Punctuator);
        Expect(tokens[38]->getType() == PPToken::Punctuator);
        Expect(tokens[40]->getType() == PPToken::Punctuator);
        Expect(tokens[42]->getType() == PPToken::Punctuator);
        Expect(tokens[44]->getType() == PPToken::Punctuator);
        Expect(tokens[46]->getType() == PPToken::Punctuator);
        Expect(tokens[48]->getType() == PPToken::Punctuator);
        Expect(tokens[50]->getType() == PPToken::Punctuator);
        Expect(tokens[52]->getType() == PPToken::Punctuator);
        Expect(tokens[54]->getType() == PPToken::Punctuator);
        Expect(tokens[56]->getType() == PPToken::Punctuator);
        Expect(tokens[58]->getType() == PPToken::Punctuator);
        Expect(tokens[60]->getType() == PPToken::Punctuator);
        Expect(tokens[62]->getType() == PPToken::Punctuator);
        Expect(tokens[64]->getType() == PPToken::Punctuator);
        Expect(tokens[66]->getType() == PPToken::Punctuator);
        Expect(tokens[68]->getType() == PPToken::Punctuator);
        Expect(tokens[70]->getType() == PPToken::Punctuator);
        Expect(tokens[72]->getType() == PPToken::Punctuator);
        Expect(tokens[74]->getType() == PPToken::Punctuator);
        Expect(tokens[76]->getType() == PPToken::Punctuator);
        Expect(tokens[78]->getType() == PPToken::Punctuator);
        Expect(tokens[80]->getType() == PPToken::Punctuator);
        Expect(tokens[82]->getType() == PPToken::Punctuator);
        Expect(tokens[84]->getType() == PPToken::Punctuator);
        Expect(tokens[86]->getType() == PPToken::Punctuator);
        Expect(tokens[88]->getType() == PPToken::Punctuator);
        Expect(tokens[90]->getType() == PPToken::Punctuator);
        Expect(tokens[92]->getType() == PPToken::Punctuator);
        Expect(tokens[94]->getType() == PPToken::Punctuator);
        Expect(tokens[96]->getType() == PPToken::Punctuator);
        Expect(tokens[98]->getType() == PPToken::Punctuator);
    })
    Test("newline", {
        std::ifstream fin("newline.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::NewLine);
        Expect(tokens[1]->getType() == PPToken::NewLine);
    })
    Test("whitespace", {
        std::ifstream fin("whitespace.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::WhiteSpace);
        Expect(tokens[1]->getType() == PPToken::Punctuator);
        Expect(tokens[2]->getType() == PPToken::NewLine);
        Expect(tokens[3]->getType() == PPToken::WhiteSpace);
        Expect(tokens[4]->getType() == PPToken::Punctuator);
        Expect(tokens[5]->getType() == PPToken::NewLine);
        Expect(tokens[6]->getType() == PPToken::Punctuator);
        Expect(tokens[7]->getType() == PPToken::WhiteSpace);
        Expect(tokens[8]->getType() == PPToken::Punctuator);
        Expect(tokens[9]->getType() == PPToken::WhiteSpace);
        Expect(tokens[10]->getType() == PPToken::NewLine);
        Expect(tokens[11]->getType() == PPToken::WhiteSpace);
        Expect(tokens[12]->getType() == PPToken::Punctuator);
    })
    Test("pp_number_int", {
        std::ifstream fin("pp_number_int.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::PPNumber);
        Expect(tokens[2]->getType() == PPToken::PPNumber);
        Expect(tokens[4]->getType() == PPToken::PPNumber);
        Expect(tokens[6]->getType() == PPToken::PPNumber);
        Expect(tokens[8]->getType() == PPToken::PPNumber);
        Expect(tokens[10]->getType() == PPToken::PPNumber);
        Expect(tokens[12]->getType() == PPToken::PPNumber);
        Expect(tokens[14]->getType() == PPToken::PPNumber);
        Expect(tokens[16]->getType() == PPToken::PPNumber);
        Expect(tokens[18]->getType() == PPToken::PPNumber);
        Expect(tokens[20]->getType() == PPToken::PPNumber);
        Expect(tokens[22]->getType() == PPToken::PPNumber);
        Expect(tokens[24]->getType() == PPToken::PPNumber);
        Expect(tokens[26]->getType() == PPToken::PPNumber);
        Expect(tokens[28]->getType() == PPToken::PPNumber);
        Expect(tokens[30]->getType() == PPToken::PPNumber);
        Expect(tokens[32]->getType() == PPToken::PPNumber);
        Expect(tokens[34]->getType() == PPToken::PPNumber);
    })
    Test("pp_number_float", {
        std::ifstream fin("pp_number_float.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::PPNumber);
        Expect(tokens[2]->getType() == PPToken::PPNumber);
        Expect(tokens[4]->getType() == PPToken::PPNumber);
        Expect(tokens[6]->getType() == PPToken::PPNumber);
        Expect(tokens[8]->getType() == PPToken::PPNumber);
        Expect(tokens[10]->getType() == PPToken::PPNumber);
        Expect(tokens[12]->getType() == PPToken::PPNumber);
        Expect(tokens[14]->getType() == PPToken::PPNumber);
        Expect(tokens[16]->getType() == PPToken::PPNumber);
        Expect(tokens[18]->getType() == PPToken::PPNumber);
        Expect(tokens[20]->getType() == PPToken::PPNumber);
        Expect(tokens[22]->getType() == PPToken::PPNumber);
        Expect(tokens[24]->getType() == PPToken::PPNumber);
        Expect(tokens[26]->getType() == PPToken::PPNumber);
        Expect(tokens[28]->getType() == PPToken::PPNumber);
        Expect(tokens[30]->getType() == PPToken::PPNumber);
    })
    Test("identifier", {
        std::ifstream fin("identifier.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::Identifier);
        Expect(tokens[2]->getType() == PPToken::Identifier);
        Expect(tokens[4]->getType() == PPToken::Identifier);
        Expect(tokens[6]->getType() == PPToken::Identifier);
        Expect(tokens[8]->getType() == PPToken::Identifier);
        Expect(tokens[10]->getType() == PPToken::Identifier);
        Expect(tokens[12]->getType() == PPToken::Identifier);
        Expect(tokens[14]->getType() == PPToken::Identifier);
    })
    Test("character", {
        std::ifstream fin("character.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::CharConst);
        Expect(tokens[2]->getType() == PPToken::CharConst);
        Expect(tokens[4]->getType() == PPToken::CharConst);
        Expect(tokens[6]->getType() == PPToken::CharConst);
        Expect(tokens[8]->getType() == PPToken::CharConst);
        Expect(tokens[10]->getType() == PPToken::CharConst);
        Expect(tokens[12]->getType() == PPToken::CharConst);
        Expect(tokens[14]->getType() == PPToken::CharConst);
        Expect(tokens[16]->getType() == PPToken::CharConst);
        Expect(tokens[18]->getType() == PPToken::CharConst);
        Expect(tokens[20]->getType() == PPToken::CharConst);
        Expect(tokens[22]->getType() == PPToken::CharConst);
        Expect(tokens[24]->getType() == PPToken::CharConst);
        Expect(tokens[26]->getType() == PPToken::CharConst);
        Expect(tokens[28]->getType() == PPToken::CharConst);
        Expect(tokens[30]->getType() == PPToken::CharConst);
        Expect(tokens[32]->getType() == PPToken::CharConst);
        Expect(tokens[34]->getType() == PPToken::CharConst);
        Expect(tokens[36]->getType() == PPToken::CharConst);
        Expect(tokens[38]->getType() == PPToken::CharConst);
        Expect(tokens[40]->getType() == PPToken::CharConst);
    })
    Test("string_literal", {
        std::ifstream fin("string_literal.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::StringLiteral);
        Expect(tokens[2]->getType() == PPToken::StringLiteral);
        Expect(tokens[4]->getType() == PPToken::StringLiteral);
        Expect(tokens[6]->getType() == PPToken::StringLiteral);
        Expect(tokens[8]->getType() == PPToken::StringLiteral);
    })
    Test("header_name", {
        std::ifstream fin("header_name.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::HeaderName);
        Expect(tokens[2]->getType() == PPToken::HeaderName);
    })
    Test("comment", {
        std::ifstream fin("comment.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPToken lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPToken::HeaderName);
        Expect(tokens[1]->getType() == PPToken::LineComment);
        Expect(tokens[3]->getType() == PPToken::HeaderName);
        Expect(tokens[4]->getType() == PPToken::BlockComment);
        Expect(tokens[5]->getType() == PPToken::HeaderName);
        Expect(tokens[7]->getType() == PPToken::BlockComment);
        Expect(tokens[8]->getType() == PPToken::PPNumber);
        Expect(tokens[10]->getType() == PPToken::BlockComment);
        Expect(tokens[11]->getType() == PPToken::Punctuator);
        Expect(tokens[12]->getType() == PPToken::Punctuator);
    })
};