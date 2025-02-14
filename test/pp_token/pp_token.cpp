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
#include <PPLexer.h>

using namespace WasmVM;
using namespace Testing;

static 

Suite pp_token {
    Test("punctuator", {
        std::ifstream fin("punctuator.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::Punctuator);
        Expect(tokens[2]->getType() == PPLexer::Punctuator);
        Expect(tokens[4]->getType() == PPLexer::Punctuator);
        Expect(tokens[6]->getType() == PPLexer::Punctuator);
        Expect(tokens[8]->getType() == PPLexer::ParenL);
        Expect(tokens[10]->getType() == PPLexer::ParenR);
        Expect(tokens[12]->getType() == PPLexer::Punctuator);
        Expect(tokens[14]->getType() == PPLexer::Punctuator);
        Expect(tokens[16]->getType() == PPLexer::Punctuator);
        Expect(tokens[18]->getType() == PPLexer::Punctuator);
        Expect(tokens[20]->getType() == PPLexer::Punctuator);
        Expect(tokens[22]->getType() == PPLexer::Punctuator);
        Expect(tokens[24]->getType() == PPLexer::Punctuator);
        Expect(tokens[26]->getType() == PPLexer::Punctuator);
        Expect(tokens[28]->getType() == PPLexer::Punctuator);
        Expect(tokens[30]->getType() == PPLexer::Punctuator);
        Expect(tokens[32]->getType() == PPLexer::Punctuator);
        Expect(tokens[34]->getType() == PPLexer::Punctuator);
        Expect(tokens[36]->getType() == PPLexer::Punctuator);
        Expect(tokens[38]->getType() == PPLexer::Punctuator);
        Expect(tokens[40]->getType() == PPLexer::Punctuator);
        Expect(tokens[42]->getType() == PPLexer::Punctuator);
        Expect(tokens[44]->getType() == PPLexer::Punctuator);
        Expect(tokens[46]->getType() == PPLexer::Punctuator);
        Expect(tokens[48]->getType() == PPLexer::Punctuator);
        Expect(tokens[50]->getType() == PPLexer::Punctuator);
        Expect(tokens[52]->getType() == PPLexer::Punctuator);
        Expect(tokens[54]->getType() == PPLexer::Punctuator);
        Expect(tokens[56]->getType() == PPLexer::Punctuator);
        Expect(tokens[58]->getType() == PPLexer::Punctuator);
        Expect(tokens[60]->getType() == PPLexer::Punctuator);
        Expect(tokens[62]->getType() == PPLexer::Punctuator);
        Expect(tokens[64]->getType() == PPLexer::Punctuator);
        Expect(tokens[66]->getType() == PPLexer::Punctuator);
        Expect(tokens[68]->getType() == PPLexer::Punctuator);
        Expect(tokens[70]->getType() == PPLexer::Punctuator);
        Expect(tokens[72]->getType() == PPLexer::Punctuator);
        Expect(tokens[74]->getType() == PPLexer::Ellipsis);
        Expect(tokens[76]->getType() == PPLexer::Punctuator);
        Expect(tokens[78]->getType() == PPLexer::Punctuator);
        Expect(tokens[80]->getType() == PPLexer::Punctuator);
        Expect(tokens[82]->getType() == PPLexer::Punctuator);
        Expect(tokens[84]->getType() == PPLexer::Punctuator);
        Expect(tokens[86]->getType() == PPLexer::Punctuator);
        Expect(tokens[88]->getType() == PPLexer::Punctuator);
        Expect(tokens[90]->getType() == PPLexer::Punctuator);
        Expect(tokens[92]->getType() == PPLexer::Punctuator);
        Expect(tokens[94]->getType() == PPLexer::Punctuator);
        Expect(tokens[96]->getType() == PPLexer::Punctuator);
        Expect(tokens[98]->getType() == PPLexer::Comma);
    })
    Test("newline", {
        std::ifstream fin("newline.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::NewLine);
        Expect(tokens[1]->getType() == PPLexer::NewLine);
    })
    Test("whitespace", {
        std::ifstream fin("whitespace.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::WhiteSpace);
        Expect(tokens[1]->getType() == PPLexer::ParenL);
        Expect(tokens[2]->getType() == PPLexer::NewLine);
        Expect(tokens[3]->getType() == PPLexer::WhiteSpace);
        Expect(tokens[4]->getType() == PPLexer::ParenR);
        Expect(tokens[5]->getType() == PPLexer::NewLine);
        Expect(tokens[6]->getType() == PPLexer::ParenL);
        Expect(tokens[7]->getType() == PPLexer::WhiteSpace);
        Expect(tokens[8]->getType() == PPLexer::ParenR);
        Expect(tokens[9]->getType() == PPLexer::WhiteSpace);
        Expect(tokens[10]->getType() == PPLexer::NewLine);
        Expect(tokens[11]->getType() == PPLexer::WhiteSpace);
        Expect(tokens[12]->getType() == PPLexer::Punctuator);
    })
    Test("pp_number_int", {
        std::ifstream fin("pp_number_int.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::PPNumber);
        Expect(tokens[2]->getType() == PPLexer::PPNumber);
        Expect(tokens[4]->getType() == PPLexer::PPNumber);
        Expect(tokens[6]->getType() == PPLexer::PPNumber);
        Expect(tokens[8]->getType() == PPLexer::PPNumber);
        Expect(tokens[10]->getType() == PPLexer::PPNumber);
        Expect(tokens[12]->getType() == PPLexer::PPNumber);
        Expect(tokens[14]->getType() == PPLexer::PPNumber);
        Expect(tokens[16]->getType() == PPLexer::PPNumber);
        Expect(tokens[18]->getType() == PPLexer::PPNumber);
        Expect(tokens[20]->getType() == PPLexer::PPNumber);
        Expect(tokens[22]->getType() == PPLexer::PPNumber);
        Expect(tokens[24]->getType() == PPLexer::PPNumber);
        Expect(tokens[26]->getType() == PPLexer::PPNumber);
        Expect(tokens[28]->getType() == PPLexer::PPNumber);
        Expect(tokens[30]->getType() == PPLexer::PPNumber);
        Expect(tokens[32]->getType() == PPLexer::PPNumber);
        Expect(tokens[34]->getType() == PPLexer::PPNumber);
    })
    Test("pp_number_float", {
        std::ifstream fin("pp_number_float.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::PPNumber);
        Expect(tokens[2]->getType() == PPLexer::PPNumber);
        Expect(tokens[4]->getType() == PPLexer::PPNumber);
        Expect(tokens[6]->getType() == PPLexer::PPNumber);
        Expect(tokens[8]->getType() == PPLexer::PPNumber);
        Expect(tokens[10]->getType() == PPLexer::PPNumber);
        Expect(tokens[12]->getType() == PPLexer::PPNumber);
        Expect(tokens[14]->getType() == PPLexer::PPNumber);
        Expect(tokens[16]->getType() == PPLexer::PPNumber);
        Expect(tokens[18]->getType() == PPLexer::PPNumber);
        Expect(tokens[20]->getType() == PPLexer::PPNumber);
        Expect(tokens[22]->getType() == PPLexer::PPNumber);
        Expect(tokens[24]->getType() == PPLexer::PPNumber);
        Expect(tokens[26]->getType() == PPLexer::PPNumber);
        Expect(tokens[28]->getType() == PPLexer::PPNumber);
        Expect(tokens[30]->getType() == PPLexer::PPNumber);
    })
    Test("identifier", {
        std::ifstream fin("identifier.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::Identifier);
        Expect(tokens[2]->getType() == PPLexer::Identifier);
        Expect(tokens[4]->getType() == PPLexer::Identifier);
        Expect(tokens[6]->getType() == PPLexer::Identifier);
        Expect(tokens[8]->getType() == PPLexer::Identifier);
        Expect(tokens[10]->getType() == PPLexer::Identifier);
        Expect(tokens[12]->getType() == PPLexer::Identifier);
        Expect(tokens[14]->getType() == PPLexer::Identifier);
    })
    Test("character", {
        std::ifstream fin("character.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::CharConst);
        Expect(tokens[2]->getType() == PPLexer::CharConst);
        Expect(tokens[4]->getType() == PPLexer::CharConst);
        Expect(tokens[6]->getType() == PPLexer::CharConst);
        Expect(tokens[8]->getType() == PPLexer::CharConst);
        Expect(tokens[10]->getType() == PPLexer::CharConst);
        Expect(tokens[12]->getType() == PPLexer::CharConst);
        Expect(tokens[14]->getType() == PPLexer::CharConst);
        Expect(tokens[16]->getType() == PPLexer::CharConst);
        Expect(tokens[18]->getType() == PPLexer::CharConst);
        Expect(tokens[20]->getType() == PPLexer::CharConst);
        Expect(tokens[22]->getType() == PPLexer::CharConst);
        Expect(tokens[24]->getType() == PPLexer::CharConst);
        Expect(tokens[26]->getType() == PPLexer::CharConst);
        Expect(tokens[28]->getType() == PPLexer::CharConst);
        Expect(tokens[30]->getType() == PPLexer::CharConst);
        Expect(tokens[32]->getType() == PPLexer::CharConst);
        Expect(tokens[34]->getType() == PPLexer::CharConst);
        Expect(tokens[36]->getType() == PPLexer::CharConst);
        Expect(tokens[38]->getType() == PPLexer::CharConst);
        Expect(tokens[40]->getType() == PPLexer::CharConst);
    })
    Test("string_literal", {
        std::ifstream fin("string_literal.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::StringLiteral);
        Expect(tokens[2]->getType() == PPLexer::StringLiteral);
        Expect(tokens[4]->getType() == PPLexer::StringLiteral);
        Expect(tokens[6]->getType() == PPLexer::StringLiteral);
        Expect(tokens[8]->getType() == PPLexer::StringLiteral);
    })
    Test("header_name", {
        std::ifstream fin("header_name.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::HeaderName);
        Expect(tokens[2]->getType() == PPLexer::HeaderName);
    })
    Test("comment", {
        std::ifstream fin("comment.txt");
        antlr4::ANTLRInputStream input(fin);
        WasmVM::PPLexer lexer(&input);
        antlr4::CommonTokenStream stream(&lexer);
        stream.fill();
        std::vector<antlr4::Token*> tokens = stream.getTokens();
        Expect(tokens[0]->getType() == PPLexer::HeaderName);
        Expect(tokens[1]->getType() == PPLexer::LineComment);
        Expect(tokens[3]->getType() == PPLexer::HeaderName);
        Expect(tokens[4]->getType() == PPLexer::BlockComment);
        Expect(tokens[5]->getType() == PPLexer::HeaderName);
        Expect(tokens[7]->getType() == PPLexer::BlockComment);
        Expect(tokens[8]->getType() == PPLexer::PPNumber);
        Expect(tokens[10]->getType() == PPLexer::BlockComment);
        Expect(tokens[11]->getType() == PPLexer::Punctuator);
        Expect(tokens[12]->getType() == PPLexer::Punctuator);
    })
};