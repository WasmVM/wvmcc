// Copyright (C) 2024 Luis Hsu
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
#include <PreProcessor.hpp>
#include <Error.hpp>

using namespace WasmVM;
using namespace Testing;

Suite pp_macro {
    Test("object-like", {
        PreProcessor pp(std::filesystem::path("obj_like.c"));
        PreProcessor::Token token;
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::NewLine);
        Expect(token.pos.line == 1 && token.pos.col == 28);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Identifier);
        Expect(token.pos.line == 2 && token.pos.col == 1);
        Expect(token.text == "foo");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::WhiteSpace);
        Expect(token.pos.line == 2 && token.pos.col == 4);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Punctuator);
        Expect(token.pos.line == 2 && token.pos.col == 5);
        Expect(token.text == "=");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::WhiteSpace);
        Expect(token.pos.line == 2 && token.pos.col == 6);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::PPNumber);
        Expect(token.pos.line == 2 && token.pos.col == 7);
        Expect(token.text == "1");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Punctuator);
        Expect(token.pos.line == 2 && token.pos.col == 8);
        Expect(token.text == ";");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::NewLine);
        Expect(token.pos.line == 2 && token.pos.col == 9);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::NewLine);
        Expect(token.pos.line == 3 && token.pos.col == 21);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::NewLine);
        Expect(token.pos.line == 4 && token.pos.col == 14);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Identifier);
        Expect(token.pos.line == 5 && token.pos.col == 1);
        Expect(token.text == "int");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::WhiteSpace);
        Expect(token.pos.line == 5 && token.pos.col == 4);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Identifier);
        Expect(token.pos.line == 5 && token.pos.col == 5);
        Expect(token.text == "test2");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::WhiteSpace);
        Expect(token.pos.line == 5 && token.pos.col == 10);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Punctuator);
        Expect(token.pos.line == 5 && token.pos.col == 11);
        Expect(token.text == "=");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::WhiteSpace);
        Expect(token.pos.line == 5 && token.pos.col == 12);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Identifier);
        Expect(token.pos.line == 5 && token.pos.col == 13);
        Expect(token.text == "foo2");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::WhiteSpace);
        Expect(token.pos.line == 5 && token.pos.col == 17);
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Identifier);
        Expect(token.pos.line == 5 && token.pos.col == 18);
        Expect(token.text == "val");
        token = pp.get().value();
        Expect(token.type == PreProcessor::Token::Punctuator);
        Expect(token.pos.line == 5 && token.pos.col == 21);
        Expect(token.text == ";");
    })
    // Test("function-like", {
    //     PreProcessor pp(std::filesystem::path("function_like.c"));
    //     PreProcessor::PPToken token;
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 1 && token.value().pos.col() == 16);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "int");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 1 && token.value().pos.col() == 20);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "a");
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 3 && token.value().pos.col() == 20);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "int");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 4 && token.value().pos.col() == 6);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "NAME");
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 6 && token.value().pos.col() == 6);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "float");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 6 && token.value().pos.col() == 13);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "cc");
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 7 && token.value().pos.col() == 20);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "struct");
    //     pp.get();
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 3 && token.value().pos.col() == 20);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "int");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 8 && token.value().pos.col() == 6);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "dd");
    //     pp.get();
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 10 && token.value().pos.col() == 6);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "char");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 10 && token.value().pos.col() == 12);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "ee");
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
    //     Expect(token.value().pos.line() == 10 && token.value().pos.col() == 16);
    //     Expect(((TokenType::PPNumber)token.value()).sequence == "3");
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 3 && token.value().pos.col() == 20);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "int");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 12 && token.value().pos.col() == 11);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "ff");
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
    //     Expect(token.value().pos.line() == 12 && token.value().pos.col() == 16);
    //     Expect(((TokenType::PPNumber)token.value()).sequence == "4");
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 14 && token.value().pos.col() == 6);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "unsigned");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 14 && token.value().pos.col() == 16);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "gg");
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 16 && token.value().pos.col() == 6);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "const");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 16 && token.value().pos.col() == 13);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "char");
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 16 && token.value().pos.col() == 19);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "hh");
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
    //     Expect(token.value().pos.line() == 16 && token.value().pos.col() == 23);
    //     Expect(((TokenType::PPNumber)token.value()).sequence == "5");
    //     pp.get();
    //     pp.get();

    //     Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
    //     Expect(token.value().pos.line() == 19 && token.value().pos.col() == 3);
    //     Expect(((TokenType::PPNumber)token.value()).sequence == "2");
    //     Expect((token = pp.get()).value() == TokenType::Punctuator(TokenType::Punctuator::Aster));
    //     Expect(token.value().pos.line() == 17 && token.value().pos.col() == 15);
    //     Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
    //     Expect(token.value().pos.line() == 19 && token.value().pos.col() == 6);
    //     Expect(((TokenType::PPNumber)token.value()).sequence == "9");
    //     Expect((token = pp.get()).value() == TokenType::Punctuator(TokenType::Punctuator::Aster));
    //     Expect(token.value().pos.line() == 17 && token.value().pos.col() == 15);
    //     Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
    //     Expect(token.value().pos.line() == 17 && token.value().pos.col() == 16);
    //     Expect(((TokenType::Identifier)token.value()).sequence == "g");
    //     pp.get();
    // })
    // Test("hash-operators", {
    //     PreProcessor pp(std::filesystem::path("hash_operator.c"));
    //     PreProcessor::PPToken token;
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
    //     Expect(token.value().pos.line() == 2 && token.value().pos.col() == 5);
    //     Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str");
    //     pp.get();
    //     pp.get();

    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
    //     Expect(token.value().pos.line() == 3 && token.value().pos.col() == 5);
    //     Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str \"s\"");
    //     pp.get();
    //     pp.get();

    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
    //     Expect(token.value().pos.line() == 4 && token.value().pos.col() == 5);
    //     Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str \"\\s\"");
    //     token = pp.get();
    //     pp.get();

    //     // token = pp.get();
    //     // token = pp.get();
    //     // token = pp.get();
    //     // token = pp.get();
    //     // Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
    //     // Expect(token.value().pos.line() == 6 && token.value().pos.col() == 6);
    //     // Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str , s");
    //     // token = pp.get();
    // })
};