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
    Test("punctuator", {
        PreProcessor pp(std::filesystem::path("punctuator.txt"));
        std::optional<Token> token;
        std::cout << "[" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_L);
        pp.get();
        std::cout << "]" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_R);
        pp.get();
        std::cout << "(" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_L);
        pp.get();
        std::cout << ")" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_R);
        pp.get();
        std::cout << "{" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_L);
        pp.get();
        std::cout << "}" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_R);
        pp.get();
        std::cout << "." << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Dot);
        pp.get();
        std::cout << "->" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Arrow);
        pp.get();
        std::cout << "++" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoublePlus);
        pp.get();
        std::cout << "--" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleMinus);
        pp.get();
        std::cout << "&" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Amp);
        pp.get();
        std::cout << "*" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Aster);
        pp.get();
        std::cout << "+" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Plus);
        pp.get();
        std::cout << "-" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Minus);
        pp.get();
        std::cout << "~" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Tlide);
        pp.get();
        std::cout << "!" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Exclam);
        pp.get();
        std::cout << "/" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Slash);
        pp.get();
        std::cout << "%" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Perce);
        pp.get();
        std::cout << "<<" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Shift_L);
        pp.get();
        std::cout << ">>" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Shift_R);
        pp.get();
        std::cout << "<" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Less);
        pp.get();
        std::cout << ">" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Great);
        pp.get();
        std::cout << "<=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::LessEqual);
        pp.get();
        std::cout << ">=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::GreatEqual);
        pp.get();
        std::cout << "==" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleEqual);
        pp.get();
        std::cout << "!=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::ExclamEqual);
        pp.get();
        std::cout << "^" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Caret);
        pp.get();
        std::cout << "|" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bar);
        pp.get();
        std::cout << "&&" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleAmp);
        pp.get();
        std::cout << "||" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleBar);
        pp.get();
        std::cout << "?" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Query);
        pp.get();
        std::cout << ":" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Colon);
        pp.get();
        std::cout << ";" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Semi);
        pp.get();
        std::cout << "..." << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Ellipsis);
        pp.get();
        std::cout << "=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Equal);
        pp.get();
        std::cout << "*=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::AsterEqual);
        pp.get();
        std::cout << "/=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::SlashEqual);
        pp.get();
        std::cout << "%=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::PerceEqual);
        pp.get();
        std::cout << "+=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::PlusEqual);
        pp.get();
        std::cout << "-=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::MinusEqual);
        pp.get();
        std::cout << "<<=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::ShiftEqual_L);
        pp.get();
        std::cout << ">>=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::ShiftEqual_R);
        pp.get();
        std::cout << "&=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::AmpEqual);
        pp.get();
        std::cout << "^=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::CaretEqual);
        pp.get();
        std::cout << "|=" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::BarEqual);
        pp.get();
        std::cout << "," << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Comma);
        pp.get();
        std::cout << "#" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Hash);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Hash);
        pp.get();
        std::cout << "##" << std::endl;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleHash);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleHash);
        pp.get();
        // stringized digraph
        std::cout << "# <:" << std::endl;
        pp.get(); pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Less);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Colon);
        pp.get();
        std::cout << "# :>" << std::endl;
        pp.get(); pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Colon);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Great);
        pp.get();
        std::cout << "# <%" << std::endl;
        pp.get(); pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Less);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Perce);
        pp.get();
        std::cout << "# %>" << std::endl;
        pp.get(); pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Perce);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Great);
        pp.get();
        std::cout << "# %:" << std::endl;
        pp.get(); pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Perce);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Colon);
        pp.get();
        std::cout << "# %:%:" << std::endl;
        pp.get(); pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Perce);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Colon);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Perce);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Colon);
    })
    Test("newline", {
        PreProcessor pp(std::filesystem::path("newline.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::NewLine>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 0);
        Expect((token = pp.get()) && std::holds_alternative<TokenType::NewLine>(token.value()));
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 0);
    })
    Test("whitespace", {
        PreProcessor pp(std::filesystem::path("whitespace.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_L);
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 2);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_R);
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 9);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_L);
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_R);
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 3);
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Semi);
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 3);
    })
    Test("pp_number_int", {
        PreProcessor pp(std::filesystem::path("pp_number_int.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 123);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0456);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0x789);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0xabc);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0xdef);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 0xffa);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 1234);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 4321);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 9 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 456);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 789);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 11 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 234);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 234);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 13 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 456);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 789);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 15 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 2345);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 3456);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 17 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 2345);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 18 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<intmax_t>() == 3456);
    })
    Test("pp_number_float", {
        PreProcessor pp(std::filesystem::path("pp_number_float.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 123.0);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0.123);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 123.456);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 12300);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 12300);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 9 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 11 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 1.23);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x123.p1);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 13 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1b3p2);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1a3P2);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 15 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1a3P2);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 1);
        Expect(((TokenType::PPNumber)token.value()).get<double>() == 0x.1a3P-2);
        pp.get();
    })
    Test("identifier", {
        PreProcessor pp(std::filesystem::path("identifier.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "ident");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "iden01");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "_iden");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "iden_");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "iden_8");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "hello");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "big");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::Identifier>(token.value()));
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(((TokenType::Identifier)token.value()).sequence == "zip");
        pp.get();
    })
    Test("character", {
        PreProcessor pp(std::filesystem::path("character.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == 'a');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(std::get<wchar_t>(((TokenType::CharacterConstant)token.value()).value) == L'a');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(std::get<char16_t>(((TokenType::CharacterConstant)token.value()).value) == u'a');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(std::get<char32_t>(((TokenType::CharacterConstant)token.value()).value) == U'a');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\012');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\x1b');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\'');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 8 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\"');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 9 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\?');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 10 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\\');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 11 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\a');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 12 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\b');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 13 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\f');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 14 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\t');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 15 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\v');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 16 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\r');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 17 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\n');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 18 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\u0068');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 19 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\U00000030');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 20 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\033');
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::CharacterConstant>(token.value()));
        Expect(token.value().pos.line() == 21 && token.value().pos.col() == 1);
        Expect(std::get<int>(((TokenType::CharacterConstant)token.value()).value) == '\x1a');
    })
    Test("header_name", {
        PreProcessor pp(std::filesystem::path("header_name.txt"));
        std::optional<Token> token;
        pp.get(); pp.get(); pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::HeaderName>(token.value()));
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 10);
        Expect(((TokenType::HeaderName)token.value()).sequence == "\"test.h\"");
        pp.get();
    })
    Test("string_literal", {
        PreProcessor pp(std::filesystem::path("string_literal.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 1);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "abcd");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        Expect(std::get<std::string>(((TokenType::StringLiteral)token.value()).value) == "a\\b\tc\nd");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect(std::get<std::u8string>(((TokenType::StringLiteral)token.value()).value) == u8"qwer");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 1);
        Expect(std::get<std::u16string>(((TokenType::StringLiteral)token.value()).value) == u"\01a\01b");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 1);
        Expect(std::get<std::wstring>(((TokenType::StringLiteral)token.value()).value) == L"\1a\2b");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect(std::get<std::u32string>(((TokenType::StringLiteral)token.value()).value) == U"\1a\2b");
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 7 && token.value().pos.col() == 1);
        Expect(std::get<std::u16string>(((TokenType::StringLiteral)token.value()).value) == u"\1a   \2b");
    })
    Test("comment", {
        PreProcessor pp(std::filesystem::path("comment.txt"));
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        // Single line
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect(token.value().pos.line() == 1 && token.value().pos.col() == 7);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 1);
        // Quoted
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 7);
        Expect((token = pp.get()) && std::holds_alternative<TokenType::StringLiteral>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 19);
        pp.get();
        // Multiple lines
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect(token.value().pos.line() == 3 && token.value().pos.col() == 1);
        Expect((token = pp.get()) && std::holds_alternative<TokenType::PPNumber>(token.value()));
        Expect(token.value().pos.line() == 5 && token.value().pos.col() == 3);
        pp.get();
        Expect((token = pp.get()) && std::holds_alternative<TokenType::WhiteSpace>(token.value()));
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 1);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Aster);
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 20);
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Slash);
        Expect(token.value().pos.line() == 6 && token.value().pos.col() == 21);
    })
};