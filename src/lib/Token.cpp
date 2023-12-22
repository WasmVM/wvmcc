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

#include <Token.hpp>
#include <Util.hpp>
#include <Error.hpp>
#include <regex>

using namespace WasmVM;

std::ostream& operator<<(std::ostream& os, Token& token){
    std::visit(overloaded {
        [&](TokenType::Punctuator& tok){
            switch(tok.type){
                case TokenType::Punctuator::Hash :
                    os << '#';
                break;
                case TokenType::Punctuator::Bracket_L :
                    os << '[';
                break;
                case TokenType::Punctuator::Bracket_R :
                    os << ']';
                break;
                case TokenType::Punctuator::Paren_L :
                    os << '(';
                break;
                case TokenType::Punctuator::Paren_R :
                    os << ')';
                break;
                case TokenType::Punctuator::Brace_L :
                    os << '{';
                break;
                case TokenType::Punctuator::Brace_R :
                    os << '}';
                break;
                case TokenType::Punctuator::Semi :
                    os << ';';
                break;
                case TokenType::Punctuator::Tlide :
                    os << '~';
                break;
                case TokenType::Punctuator::Query :
                    os << '?';
                break;
                case TokenType::Punctuator::Comma :
                    os << ',';
                break;
                case TokenType::Punctuator::DoubleHash :
                    os << "##";
                break;
                case TokenType::Punctuator::Dot :
                    os << ".";
                break;
                case TokenType::Punctuator::Ellipsis :
                    os << "...";
                break;
                case TokenType::Punctuator::Aster :
                    os << "*";
                break;
                case TokenType::Punctuator::AsterEqual :
                    os << "*=";
                break;
                case TokenType::Punctuator::Caret :
                    os << "^";
                break;
                case TokenType::Punctuator::CaretEqual :
                    os << "^=";
                break;
                case TokenType::Punctuator::Exclam :
                    os << "!";
                break;
                case TokenType::Punctuator::ExclamEqual :
                    os << "!=";
                break;
                case TokenType::Punctuator::Equal :
                    os << "=";
                break;
                case TokenType::Punctuator::DoubleEqual :
                    os << "==";
                break;
                case TokenType::Punctuator::Slash :
                    os << "/";
                break;
                case TokenType::Punctuator::SlashEqual :
                    os << "/=";
                break;
                case TokenType::Punctuator::Colon :
                    os << ":";
                break;
                case TokenType::Punctuator::Plus :
                    os << "+";
                break;
                case TokenType::Punctuator::DoublePlus :
                    os << "++";
                break;
                case TokenType::Punctuator::PlusEqual :
                    os << "+=";
                break;
                case TokenType::Punctuator::Amp :
                    os << "&";
                break;
                case TokenType::Punctuator::DoubleAmp :
                    os << "&&";
                break;
                case TokenType::Punctuator::AmpEqual :
                    os << "&=";
                break;
                case TokenType::Punctuator::Bar :
                    os << "|";
                break;
                case TokenType::Punctuator::DoubleBar :
                    os << "||";
                break;
                case TokenType::Punctuator::BarEqual :
                    os << "|=";
                break;
                case TokenType::Punctuator::Minus :
                    os << "-";
                break;
                case TokenType::Punctuator::DoubleMinus :
                    os << "--";
                break;
                case TokenType::Punctuator::MinusEqual :
                    os << "-=";
                break;
                case TokenType::Punctuator::Arrow :
                    os << "->";
                break;
                case TokenType::Punctuator::Shift_R :
                    os << ">>";
                break;
                case TokenType::Punctuator::Great :
                    os << ">";
                break;
                case TokenType::Punctuator::GreatEqual :
                    os << ">=";
                break;
                case TokenType::Punctuator::ShiftEqual_R :
                    os << ">>=";
                break;
                case TokenType::Punctuator::Perce :
                    os << "%";
                break;
                case TokenType::Punctuator::PerceEqual :
                    os << "%=";
                break;
                case TokenType::Punctuator::Shift_L :
                    os << "<<";
                break;
                case TokenType::Punctuator::ShiftEqual_L :
                    os << "<<=";
                break;
                case TokenType::Punctuator::Less :
                    os << "<";
                break;
                case TokenType::Punctuator::LessEqual :
                    os << "<=";
                break;
            }
        },
        [&](TokenType::NewLine&){
            os << std::endl;
        },
        [&](TokenType::WhiteSpace&){
            os << " ";
        },
        [&](TokenType::PPNumber& tok){
            os << tok.sequence;
        },
        [&](TokenType::Identifier& tok){
            os << tok.sequence;
        }
    }, token);
    return os;
}

template<>
intmax_t TokenType::PPNumber::get<intmax_t>(){
    int base = 10;
    if(sequence.starts_with("0x") || sequence.starts_with("0X")){
        base = 16;
    }else if(sequence.starts_with("0")){
        base = 8;
    }
    return std::stoll(sequence, nullptr, base);
}

template<>
double TokenType::PPNumber::get<double>(){
    return std::stod(sequence);
}
