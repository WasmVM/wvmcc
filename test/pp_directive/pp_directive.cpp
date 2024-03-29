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

#include <vector>

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
    Test("include", {
        std::vector<std::filesystem::path> include_paths {"."}; 
        PreProcessor pp(std::filesystem::path("include/include.c"), include_paths);
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(token.value().pos.path.filename() == "in.h");
        Expect(((TokenType::StringLiteral)token.value()).sequence == "\"in\"");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(token.value().pos.path.filename() == "out.h");
        Expect(((TokenType::StringLiteral)token.value()).sequence == "\"out\"");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(token.value().pos.path.filename() == "out.h");
        Expect(((TokenType::StringLiteral)token.value()).sequence == "\"out\"");
        pp.get();
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(token.value().pos.path.filename() == "out.h");
        Expect(((TokenType::StringLiteral)token.value()).sequence == "\"out\"");
        pp.get();
        pp.get();
    })
    Test("undef", {
        PreProcessor pp(std::filesystem::path("undef.c"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 11);
        Expect(((TokenType::PPNumber)token.value()).sequence == "3");
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "X");
        pp.get();
    })
    Test("line", {
        PreProcessor pp(std::filesystem::path("line.c"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(token.value().pos.path.filename() == "line.c");
        Expect(((TokenType::PPNumber)token.value()).sequence == "1");
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 1);
        Expect(token.value().pos.path.filename() == "line.c");
        Expect(((TokenType::PPNumber)token.value()).sequence == "2");
        pp.get();

        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(token.value().pos.path.filename() == "test.c");
        Expect(((TokenType::PPNumber)token.value()).sequence == "3");
        pp.get();
    })
    Test("error", {
        PreProcessor pp(std::filesystem::path("error.c"));
        Throw(Exception::Exception, pp.get());
    })
    Test("pragma", {
        PreProcessor pp(std::filesystem::path("pragma.c"));
        PreProcessor::PPToken token;
        pp.get();
        Expect(pp.pragma.fp_contract.has_value() && (pp.pragma.fp_contract.value() == true));
        pp.get();

        pp.get();
        Expect(pp.pragma.fp_contract.has_value() && (pp.pragma.fp_contract.value() == false));
        pp.get();

        pp.get();
        Expect(!pp.pragma.fp_contract.has_value());
        pp.get();

        pp.get();
        Expect(pp.pragma.fenv_access.has_value() && (pp.pragma.fenv_access.value() == true));
        pp.get();

        pp.get();
        Expect(pp.pragma.fenv_access.has_value() && (pp.pragma.fenv_access.value() == false));
        pp.get();

        pp.get();
        Expect(!pp.pragma.fenv_access.has_value());
        pp.get();

        pp.get();
        Expect(pp.pragma.cx_limited_value.has_value() && (pp.pragma.cx_limited_value.value() == true));
        pp.get();

        pp.get();
        Expect(pp.pragma.cx_limited_value.has_value() && (pp.pragma.cx_limited_value.value() == false));
        pp.get();

        pp.get();
        Expect(!pp.pragma.cx_limited_value.has_value());
        pp.get();
    })
};