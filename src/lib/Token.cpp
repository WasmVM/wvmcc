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
#include <sstream>

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
        },
        [&](TokenType::CharacterConstant& tok){
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

TokenType::CharacterConstant::CharacterConstant(std::string sequence) : sequence(sequence){
    auto seq_it = sequence.begin();
    int width = sizeof(char);
    if(*seq_it != '\''){
        switch(*seq_it){
            case 'L':
                width = sizeof(wchar_t);
            break;
            case 'u':
                width = sizeof(char16_t);
            break;
            case 'U':
                width = sizeof(char32_t);
            break;
            default:
                throw Exception::Exception("unknown character constant prefix");
            break;
        }
        ++seq_it;
    }
    ++seq_it;
    if(*seq_it == '\''){
        Exception::Warning("null charecter is better specified by '\\0' instead of ''");
    }
    uintmax_t val = 0;
    for(int w = 0; w < width && *seq_it != '\''; ++w){
        uintmax_t char_val = 0;
        if(*seq_it == '\\'){
            // escape
            ++seq_it;
            switch(*seq_it){
                // Octal
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':{
                    uint8_t oc = 0;
                    for(int c = 0; (c < 3) && (*seq_it >= '0') && (*seq_it <= '7'); ++c, ++seq_it){
                        oc = (oc << 3) + (*seq_it - '0');
                    }
                    char_val = oc;
                }break;
                case 'x':{
                    ++seq_it;
                    for(int c = 0; (c < CHAR_BIT / 4) && std::isxdigit(*seq_it); ++c, ++seq_it){
                        int lower = std::tolower(*seq_it);
                        char_val = (char_val << 4) + (std::isdigit(lower) ? (*seq_it - '0') : (*seq_it - 'a' + 10));
                    }
                }break;
                case '\'':
                    char_val = '\'';
                    ++seq_it;
                break;
                case '\"':
                    char_val = '\"';
                    ++seq_it;
                break;
                case '\?':
                    char_val = '\?';
                    ++seq_it;
                break;
                case '\\':
                    char_val = '\\';
                    ++seq_it;
                break;
                case 'a':
                    char_val = '\a';
                    ++seq_it;
                break;
                case 'b':
                    char_val = '\b';
                    ++seq_it;
                break;
                case 'f':
                    char_val = '\f';
                    ++seq_it;
                break;
                case 'n':
                    char_val = '\n';
                    ++seq_it;
                break;
                case 'r':
                    char_val = '\r';
                    ++seq_it;
                break;
                case 't':
                    char_val = '\t';
                    ++seq_it;
                break;
                case 'v':
                    char_val = '\v';
                    ++seq_it;
                break;
            }
        }else{
            char_val = *seq_it;
            ++seq_it;
        }
        val = (val << (w * CHAR_BIT)) + (char_val & ((1 << CHAR_BIT) - 1));
    }
    if(*seq_it != '\''){
        Exception::Warning("charecters exceed charecter constant range are discarded");
    }
    switch(sequence.front()){
        case 'L':
            value.emplace<wchar_t>((wchar_t)val);
        break;
        case 'u':
            value.emplace<char16_t>(val);
        break;
        case 'U':
            value.emplace<char32_t>(val);
        break;
        default:
            value.emplace<int>(val);
        break;
    }
}