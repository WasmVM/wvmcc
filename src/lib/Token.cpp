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
#include <string>
#include <regex>
#include <sstream>
#include <cctype>

using namespace WasmVM;

static std::ostream& print_char(std::ostream& os, int ch, int width = sizeof(char)){
    switch(ch){
        case '\\':
            return os << "\\\\";
        case '\t':
            return os << "\\t";
        case '\n':
            return os << "\\n";
        case '\v':
            return os << "\\v";
        case '\f':
            return os << "\\f";
        case '\r':
            return os << "\\r";
        default:
            if(std::isprint(ch)){
                return os << (char)ch;
            }else{
                os << "\\x";
                bool non_zero = false;
                for(int i = width * (CHAR_BIT / 4) - 1; i >= 0; --i){
                    unsigned int c = (ch >> (4 * i)) & 0xf;
                    if(c > 0){
                        non_zero = true;
                        if(c >= 10){
                            os << (char)('a' + (c - 10));
                        }else{
                            os << c;
                        }
                    }else if(non_zero || (i == 0)){
                        os << "0";
                    }

                }
                return os;
            }
    }
}

static uintmax_t retrieve_char(std::string::iterator& seq_it){
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
                unsigned char oc = 0;
                for(int c = 0; (c < 3) && (*seq_it >= '0') && (*seq_it <= '7'); ++c, ++seq_it){
                    oc = (oc << 3) + (*seq_it - '0');
                }
                char_val = oc;
            }break;
            case 'x':{
                ++seq_it;
                for(int c = 0; (c < CHAR_BIT / 4) && std::isxdigit(*seq_it); ++c, ++seq_it){
                    int lower = std::tolower(*seq_it);
                    char_val = (char_val << 4) + (std::isdigit(lower) ? (lower - '0') : (lower - 'a' + 10));
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
            // universal character names
            case 'u':
                for(int i = 0; i < 4; ++i){
                    ++seq_it;
                    if(std::isxdigit(*seq_it)){
                        int lower = std::tolower(*seq_it);
                        char_val = (char_val << 4) + (std::isdigit(lower) ? (lower - '0') : (lower - 'a' + 10));
                    }else{
                        throw Exception::Exception("invalid universal character name");
                    }
                }
            break;
            case 'U':
                for(int i = 0; i < 8; ++i){
                    ++seq_it;
                    if(std::isxdigit(*seq_it)){
                        int lower = std::tolower(*seq_it);
                        char_val = (char_val << 4) + (std::isdigit(lower) ? (lower - '0') : (lower - 'a' + 10));
                    }else{
                        throw Exception::Exception("invalid universal character name");
                    }
                }
            break;
        }
    }else{
        char_val = *seq_it;
        ++seq_it;
    }
    return char_val;
}

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
            std::visit(overloaded {
                [&](int val){
                    os << "'";
                    print_char(os, val);
                },
                [&](wchar_t val){
                    os << "L'";
                    print_char(os, val, sizeof(wchar_t));
                },
                [&](char16_t val){
                    os << "u'";
                    print_char(os, val, sizeof(char16_t));
                },
                [&](char32_t val){
                    os << "U'";
                    print_char(os, val, sizeof(char32_t));
                }
            }, tok.value);
            os << "'";
        },
        [&](TokenType::HeaderName& tok){
            os << tok.sequence;
        },
        [&](TokenType::StringLiteral& tok){
            std::visit(overloaded {
                [&](std::string str){
                    os << "\"";
                    for(char ch : str){
                        print_char(os, ch, sizeof(char));
                    }
                },
                [&](std::u8string str){
                    os << "u8\"";
                    for(char8_t ch : str){
                        print_char(os, ch, sizeof(char8_t));
                    }
                },
                [&](std::wstring str){
                    os << "L\"";
                    for(wchar_t ch : str){
                        print_char(os, ch, sizeof(wchar_t));
                    }
                },
                [&](std::u16string str){
                    os << "u\"";
                    for(wchar_t ch : str){
                        print_char(os, ch, sizeof(char16_t));
                    }
                },
                [&](std::u32string str){
                    os << "U\"";
                    for(wchar_t ch : str){
                        print_char(os, ch, sizeof(char32_t));
                    }
                }
            }, tok.value);
            os << "\"";
        },
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

TokenType::CharacterConstant::CharacterConstant(std::string sequence){
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
        val = (val << CHAR_BIT) + (retrieve_char(seq_it) & ((1 << CHAR_BIT) - 1));
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

enum class StringPrefix {
    none, L, u, u8, U
};

TokenType::StringLiteral::StringLiteral(std::string sequence){
    auto seq_it = sequence.begin();
    int width = sizeof(char);
    if(*seq_it != '\"'){
        switch(*seq_it){
            case 'L':
                width = sizeof(wchar_t);
                value.emplace<std::wstring>();
            break;
            case 'u':
                if(seq_it[1] == '8'){
                    width = sizeof(char8_t);
                    value.emplace<std::u8string>();
                    ++seq_it;
                }else{
                    width = sizeof(char16_t);
                    value.emplace<std::u16string>();
                }
            break;
            case 'U':
                width = sizeof(char32_t);
                value.emplace<std::u32string>();
            break;
            default:
                throw Exception::Exception("unknown string literal prefix");
            break;
        }
        ++seq_it;
    }else{
        value.emplace<std::string>();
    }
    ++seq_it;
    while(*seq_it != '\"'){
        std::visit(overloaded {
            [&](std::string& str){
                str += (char)retrieve_char(seq_it);
            },
            [&](std::wstring& str){
                str += (wchar_t)retrieve_char(seq_it);
            },
            [&](std::u8string& str){
                str += (char8_t)retrieve_char(seq_it);
            },
            [&](std::u16string& str){
                str += (char16_t)retrieve_char(seq_it);
            },            
            [&](std::u32string& str){
                str += (char32_t)retrieve_char(seq_it);
            }
        }, value);
    }
}

bool TokenType::operator==(const NewLine&, const NewLine&){
    return true;
}
bool TokenType::operator==(const WhiteSpace&, const WhiteSpace&){
    return true;
}
bool TokenType::operator==(const PPNumber& a, const PPNumber& b){
    return a.sequence == b.sequence;
}
bool TokenType::operator==(const Identifier& a, const Identifier& b){
    return a.sequence == b.sequence;
}
bool TokenType::operator==(const CharacterConstant& a, const CharacterConstant& b){
    return a.value == b.value;
}
bool TokenType::operator==(const StringLiteral& a, const StringLiteral& b){
    return a.value == b.value;
}
bool TokenType::operator==(const HeaderName& a, const HeaderName& b){
    return a.sequence == b.sequence;
}