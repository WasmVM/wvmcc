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

Suite preprocessor {
    Test("punctuator", {
        PreProcessor pp("punctuator.txt");
        std::optional<Token> token;
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bracket_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Paren_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Brace_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Dot);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Arrow);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoublePlus);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleMinus);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Amp);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Aster);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Plus);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Minus);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Tlide);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Exclam);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Slash);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Perce);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Shift_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Shift_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Less);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Great);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::LessEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::GreatEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::ExclamEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Caret);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Bar);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleAmp);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleBar);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Query);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Colon);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Semi);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Ellipsis);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Equal);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::AsterEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::SlashEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::PerceEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::PlusEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::MinusEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::ShiftEqual_L);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::ShiftEqual_R);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::AmpEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::CaretEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::BarEqual);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Comma);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Hash);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Hash);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleHash);
        pp.get();
        Expect((token = pp.get()) && ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::DoubleHash);
    })
    Test("newline", {
        PreProcessor pp("newline.txt");
        std::optional<Token> token;
        Expect((token = pp.get()) && std::holds_alternative<TokenType::NewLine>(token.value()));
        Expect(token.value().pos.line() == 2 && token.value().pos.col() == 0);
        Expect((token = pp.get()) && std::holds_alternative<TokenType::NewLine>(token.value()));
        Expect(token.value().pos.line() == 4 && token.value().pos.col() == 0);
    })
};