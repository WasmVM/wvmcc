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
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 13);
        Expect(((TokenType::Identifier)token.value()).sequence == "const");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 19);
        Expect(((TokenType::Identifier)token.value()).sequence == "int");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 23);
        Expect(((TokenType::Identifier)token.value()).sequence == "test1");
        token = pp.get();
        token = pp.get();
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "int");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 5);
        Expect(((TokenType::Identifier)token.value()).sequence == "test2");
        Expect((token = pp.get()).value() == TokenType::Punctuator(TokenType::Punctuator::Equal));
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 11);
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 14);
        Expect(((TokenType::Identifier)token.value()).sequence == "test1");
        Expect((token = pp.get()).value() == TokenType::Punctuator(TokenType::Punctuator::Plus));
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 20);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 13);
        Expect(((TokenType::PPNumber)token.value()).sequence == "4");
    })
    Test("function-like", {
        PreProcessor pp(std::filesystem::path("function_like.c"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 16);
        Expect(((TokenType::Identifier)token.value()).sequence == "int");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 20);
        Expect(((TokenType::Identifier)token.value()).sequence == "a");
        token = pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 20);
        Expect(((TokenType::Identifier)token.value()).sequence == "int");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 6);
        Expect(((TokenType::Identifier)token.value()).sequence == "NAME");
        token = pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 6);
        Expect(((TokenType::Identifier)token.value()).sequence == "float");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 13);
        Expect(((TokenType::Identifier)token.value()).sequence == "cc");
        token = pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 20);
        Expect(((TokenType::Identifier)token.value()).sequence == "struct");
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 20);
        Expect(((TokenType::Identifier)token.value()).sequence == "int");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 6);
        Expect(((TokenType::Identifier)token.value()).sequence == "dd");
        token = pp.get();
        token = pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 6);
        Expect(((TokenType::Identifier)token.value()).sequence == "char");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 12);
        Expect(((TokenType::Identifier)token.value()).sequence == "ee");
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 16);
        Expect(((TokenType::PPNumber)token.value()).sequence == "3");
        token = pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 20);
        Expect(((TokenType::Identifier)token.value()).sequence == "int");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 11);
        Expect(((TokenType::Identifier)token.value()).sequence == "ff");
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 16);
        Expect(((TokenType::PPNumber)token.value()).sequence == "4");
        token = pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 6);
        Expect(((TokenType::Identifier)token.value()).sequence == "unsigned");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 16);
        Expect(((TokenType::Identifier)token.value()).sequence == "gg");
        token = pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 6);
        Expect(((TokenType::Identifier)token.value()).sequence == "const");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 13);
        Expect(((TokenType::Identifier)token.value()).sequence == "char");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 19);
        Expect(((TokenType::Identifier)token.value()).sequence == "hh");
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 23);
        Expect(((TokenType::PPNumber)token.value()).sequence == "5");
        token = pp.get();
    })
    Test("hash-operators", {
        PreProcessor pp(std::filesystem::path("hash_operator.c"));
        PreProcessor::PPToken token;
        token = pp.get();
        token = pp.get();
        token = pp.get();
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 5);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str");
        token = pp.get();

        token = pp.get();
        token = pp.get();
        token = pp.get();
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 5);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str \"s\"");
        token = pp.get();

        token = pp.get();
        token = pp.get();
        token = pp.get();
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 5);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str \"\\s\"");
        token = pp.get();

        token = pp.get();
        token = pp.get();
        token = pp.get();
        token = pp.get();
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 6);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "str , s");
        token = pp.get();
    })
};