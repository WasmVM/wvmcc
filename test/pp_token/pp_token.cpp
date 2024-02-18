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
#include <PreProcessor.hpp>

using namespace WasmVM;
using namespace Testing;

Suite pp_token {
    // Test("punctuator", {
    //     PreProcessor pp(std::filesystem::path("punctuator.txt"));
    //     std::cout << "[" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Bracket_L));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Bracket_L));
    //     std::cout << "]" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Bracket_R));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Bracket_R));
    //     std::cout << "(" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Paren_L));
    //     std::cout << ")" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Paren_R));
    //     std::cout << "{" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Brace_L));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Brace_L));
    //     std::cout << "}" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Brace_R));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Brace_R));
    //     std::cout << "." << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Dot));
    //     std::cout << "->" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Arrow));
    //     std::cout << "++" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::DoublePlus));
    //     std::cout << "--" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::DoubleMinus));
    //     std::cout << "&" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Amp));
    //     std::cout << "*" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Aster));
    //     std::cout << "+" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Plus));
    //     std::cout << "-" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Minus));
    //     std::cout << "~" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Tlide));
    //     std::cout << "!" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Exclam));
    //     std::cout << "/" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Slash));
    //     std::cout << "%" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Perce));
    //     std::cout << "<<" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Shift_L));
    //     std::cout << ">>" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Shift_R));
    //     std::cout << "<" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Less));
    //     std::cout << ">" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Great));
    //     std::cout << "<=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::LessEqual));
    //     std::cout << ">=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::GreatEqual));
    //     std::cout << "==" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::DoubleEqual));
    //     std::cout << "!=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::ExclamEqual));
    //     std::cout << "^" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Caret));
    //     std::cout << "|" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Bar));
    //     std::cout << "&&" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::DoubleAmp));
    //     std::cout << "||" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::DoubleBar));
    //     std::cout << "?" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Query));
    //     std::cout << ":" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Colon));
    //     std::cout << ";" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Semi));
    //     std::cout << "..." << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Ellipsis));
    //     std::cout << "=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Equal));
    //     std::cout << "*=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::AsterEqual));
    //     std::cout << "/=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::SlashEqual));
    //     std::cout << "%=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::PerceEqual));
    //     std::cout << "+=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::PlusEqual));
    //     std::cout << "-=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::MinusEqual));
    //     std::cout << "<<=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::ShiftEqual_L));
    //     std::cout << ">>=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::ShiftEqual_R));
    //     std::cout << "&=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::AmpEqual));
    //     std::cout << "^=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::CaretEqual));
    //     std::cout << "|=" << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::BarEqual));
    //     std::cout << "," << std::endl;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Comma));
    // })
    // Test("newline", {
    //     PreProcessor pp(std::filesystem::path("newline.txt"));
    //     PreProcessor::PPToken token;
    //     Expect(pp.get() == std::nullopt);
    //     Expect(pp.get() == std::nullopt);
    //     Expect(pp.get() == std::nullopt);
    // })
    // Test("whitespace", {
    //     PreProcessor pp(std::filesystem::path("whitespace.txt"));
    //     PreProcessor::PPToken token;
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Paren_L));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Paren_R));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Paren_L));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Paren_R));
    //     Expect(pp.get().value() == TokenType::Punctuator(TokenType::Punctuator::Semi));
    // })
    Test("pp_number_int", {
        PreProcessor pp(std::filesystem::path("pp_number_int.txt"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 123);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0456);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0x789);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0xabc);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0xdef);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0xffa);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 1234);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 4321);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 9 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 456);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 789);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 11 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 234);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 234);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 13 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 456);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 789);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 15 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 2345);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 3456);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 17 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 2345);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 18 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 3456);
    })
    Test("pp_number_float", {
        PreProcessor pp(std::filesystem::path("pp_number_float.txt"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 123.0);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0.123);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 123.456);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 12300);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 12300);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 9 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 11 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x123.p1);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 13 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1b3p2);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1a3P2);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 15 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1a3P2);
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1a3P-2);
    })
    Test("identifier", {
        PreProcessor pp(std::filesystem::path("identifier.txt"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "ident");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "iden01");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "_iden");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "iden_");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "iden_8");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "hello");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "big");
        Expect((token = pp.get()) && token.hold<TokenType::Identifier>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "zip");
    })
    Test("character", {
        PreProcessor pp(std::filesystem::path("character.txt"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == 'a');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(std::get<wchar_t>(((TokenType::CharacterConstant)token.value()).value) == L'a');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(std::get<char16_t>(((TokenType::CharacterConstant)token.value()).value) == u'a');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(std::get<char32_t>(((TokenType::CharacterConstant)token.value()).value) == U'a');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\012');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\x1b');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\'');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\"');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 9 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\?');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\\');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 11 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\a');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\b');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 13 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\f');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\t');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 15 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\v');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\r');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 17 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\n');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 18 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\u0068');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 19 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\U00000030');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 20 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\033');
        Expect((token = pp.get()) && token.hold<TokenType::CharacterConstant>());
        Expect(token.value().pos.line() == 21 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\x1a');
    })
    // Test("header_name", {
    //     PreProcessor pp(std::filesystem::path("header_name.txt"));
    //     PreProcessor::PPToken token;
    //     pp.get();
    //     Expect((token = pp.get()) && token.hold<TokenType::HeaderName>());
    //     Expect(token.value().pos.line() == 1 && token.value().pos.col() == 10);
    //     Expect(((TokenType::HeaderName)token.value()).sequence == "\"test.h\"");
    // })
    Test("string_literal", {
        PreProcessor pp(std::filesystem::path("string_literal.txt"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "abcd");
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "a\\b\tc\nd");
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(std::get<std::u8string>(((TokenType::StringLiteral)token.value()).value) == u8"qwer");
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(std::get<std::u16string>(((TokenType::StringLiteral)token.value()).value) == u"\01a\01b");
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(std::get<std::wstring>(((TokenType::StringLiteral)token.value()).value) == L"\1a\2b");
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(std::get<std::u32string>(((TokenType::StringLiteral)token.value()).value) == U"\1a\2b");
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(std::get<std::u16string>(((TokenType::StringLiteral)token.value()).value) == u"\1a   \2b");
    })
    Test("comment", {
        PreProcessor pp(std::filesystem::path("comment.txt"));
        PreProcessor::PPToken token;
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        // Single line
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        pp.get();
        // Quoted
        Expect((token = pp.get()) && token.hold<TokenType::StringLiteral>());
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 19);
        // Multiple lines
        Expect((token = pp.get()) && token.hold<TokenType::PPNumber>());
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 3);
        Expect((token = pp.get()).value() == TokenType::Punctuator(TokenType::Punctuator::Aster));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 20);
        Expect((token = pp.get()).value() == TokenType::Punctuator(TokenType::Punctuator::Slash));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 21);
    })
};