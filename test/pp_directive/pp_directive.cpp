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

Suite pp_directive {
    Test("if", {
        PreProcessor pp(std::filesystem::path("if.c"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "2");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 15 && token.value().pos.col() == 5);
        Expect(((TokenType::PPNumber)token.value()).sequence == "4");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 17 && token.value().pos.col() == 9);
        Expect(((TokenType::PPNumber)token.value()).sequence == "5");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 21 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "6");
        pp.get();
        pp.get();
    })
    Test("else", {
        PreProcessor pp(std::filesystem::path("else.c"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "2");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "3");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 23 && token.value().pos.col() == 5);
        Expect(((TokenType::PPNumber)token.value()).sequence == "8");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 35 && token.value().pos.col() == 5);
        Expect(((TokenType::PPNumber)token.value()).sequence == "11");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 45 && token.value().pos.col() == 5);
        Expect(((TokenType::PPNumber)token.value()).sequence == "14");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 57 && token.value().pos.col() == 5);
        Expect(((TokenType::PPNumber)token.value()).sequence == "17");
        pp.get();
        pp.get();
    })
    Test("elif", {
        PreProcessor pp(std::filesystem::path("elif.c"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "2");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "5");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 20 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "7");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 35 && token.value().pos.col() == 5);
        Expect(((TokenType::PPNumber)token.value()).sequence == "12");
        pp.get();
        pp.get();
    })
    Test("defined", {
        PreProcessor pp(std::filesystem::path("defined.c"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "1");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "2");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 22 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "6");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 24 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).sequence == "7");
        pp.get();
        pp.get();
    })
};