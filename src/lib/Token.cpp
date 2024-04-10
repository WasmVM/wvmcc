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
#include <cinttypes>
#include <climits>
#include <unordered_set>

using namespace WasmVM;

static std::string char_to_str(int ch, int width = sizeof(char)){
    switch(ch){
        case '\\':
            return "\\\\";
        case '\t':
            return "\\t";
        case '\n':
            return "\\n";
        case '\v':
            return "\\v";
        case '\f':
            return "\\f";
        case '\r':
            return "\\r";
        default:
            if(std::isprint(ch)){
                return std::string(1, (char)ch);
            }else{
                std::string res = "\\x";
                bool non_zero = false;
                for(int i = width * (CHAR_BIT / 4) - 1; i >= 0; --i){
                    unsigned int c = (ch >> (4 * i)) & 0xf;
                    if(c > 0){
                        non_zero = true;
                        if(c >= 10){
                            res += (char)('a' + (c - 10));
                        }else{
                            res += (char)('0' + c);
                        }
                    }else if(non_zero || (i == 0)){
                        res += "0";
                    }

                }
                return res;
            }
    }
}

static std::ostream& print_char(std::ostream& os, int ch, int width = sizeof(char)){
    return os << char_to_str(ch, width);
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
        [&](TokenType::IntegerConstant& tok){
            std::visit(overloaded{
                [&](auto val){
                    os << val;
                }
            }, tok);
        },
        [&](TokenType::FloatingConstant& tok){
            os << tok;
        },       
        [&](TokenType::CharacterConstant& tok){
            std::visit(overloaded {
                [&](int val){
                    os << "'";
                    if(val == '\''){
                        os << "\\";
                    }
                    print_char(os, val);
                },
                [&](wchar_t val){
                    os << "L'";
                    if(val == L'\''){
                        os << "\\";
                    }
                    print_char(os, val, sizeof(wchar_t));
                },
                [&](char16_t val){
                    os << "u'";
                    if(val == u'\''){
                        os << "\\";
                    }
                    print_char(os, val, sizeof(char16_t));
                },
                [&](char32_t val){
                    os << "U'";
                    if(val == U'\''){
                        os << "\\";
                    }
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
                        if(ch == '\"'){
                            os << "\\";
                        }
                        print_char(os, ch, sizeof(char));
                    }
                },
                [&](std::u8string str){
                    os << "u8\"";
                    for(char8_t ch : str){
                        if(ch == '\"'){
                            os << "\\";
                        }
                        print_char(os, ch, sizeof(char8_t));
                    }
                },
                [&](std::wstring str){
                    os << "L\"";
                    for(wchar_t ch : str){
                        if(ch == L'\"'){
                            os << "\\";
                        }
                        print_char(os, ch, sizeof(wchar_t));
                    }
                },
                [&](std::u16string str){
                    os << "u\"";
                    for(char16_t ch : str){
                        if(ch == u'\"'){
                            os << "\\";
                        }
                        print_char(os, ch, sizeof(char16_t));
                    }
                },
                [&](std::u32string str){
                    os << "U\"";
                    for(char32_t ch : str){
                        if(ch == U'\"'){
                            os << "\\";
                        }
                        print_char(os, ch, sizeof(char32_t));
                    }
                }
            }, tok.value);
            os << "\"";
        },
    }, token);
    return os;
}

std::string Token::str(){
    return std::visit<std::string>(overloaded {
        [](TokenType::Punctuator& tok){
            switch(tok.type){
                case TokenType::Punctuator::Hash :
                    return "#";
                case TokenType::Punctuator::Bracket_L :
                    return "[";
                case TokenType::Punctuator::Bracket_R :
                    return "]";
                case TokenType::Punctuator::Paren_L :
                    return "(";
                case TokenType::Punctuator::Paren_R :
                    return ")";
                case TokenType::Punctuator::Brace_L :
                    return "{";
                case TokenType::Punctuator::Brace_R :
                    return "}";
                case TokenType::Punctuator::Semi :
                    return ";";
                case TokenType::Punctuator::Tlide :
                    return "~";
                case TokenType::Punctuator::Query :
                    return "?";
                case TokenType::Punctuator::Comma :
                    return ",";
                case TokenType::Punctuator::DoubleHash :
                    return "##";
                case TokenType::Punctuator::Dot :
                    return ".";
                case TokenType::Punctuator::Ellipsis :
                    return "...";
                case TokenType::Punctuator::Aster :
                    return "*";
                case TokenType::Punctuator::AsterEqual :
                    return "*=";
                case TokenType::Punctuator::Caret :
                    return "^";
                case TokenType::Punctuator::CaretEqual :
                    return "^=";
                case TokenType::Punctuator::Exclam :
                    return "!";
                case TokenType::Punctuator::ExclamEqual :
                    return "!=";
                case TokenType::Punctuator::Equal :
                    return "=";
                case TokenType::Punctuator::DoubleEqual :
                    return "==";
                case TokenType::Punctuator::Slash :
                    return "/";
                case TokenType::Punctuator::SlashEqual :
                    return "/=";
                case TokenType::Punctuator::Colon :
                    return ":";
                case TokenType::Punctuator::Plus :
                    return "+";
                case TokenType::Punctuator::DoublePlus :
                    return "++";
                case TokenType::Punctuator::PlusEqual :
                    return "+=";
                case TokenType::Punctuator::Amp :
                    return "&";
                case TokenType::Punctuator::DoubleAmp :
                    return "&&";
                case TokenType::Punctuator::AmpEqual :
                    return "&=";
                case TokenType::Punctuator::Bar :
                    return "|";
                case TokenType::Punctuator::DoubleBar :
                    return "||";
                case TokenType::Punctuator::BarEqual :
                    return "|=";
                case TokenType::Punctuator::Minus :
                    return "-";
                case TokenType::Punctuator::DoubleMinus :
                    return "--";
                case TokenType::Punctuator::MinusEqual :
                    return "-=";
                case TokenType::Punctuator::Arrow :
                    return "->";
                case TokenType::Punctuator::Shift_R :
                    return ">>";
                case TokenType::Punctuator::Great :
                    return ">";
                case TokenType::Punctuator::GreatEqual :
                    return ">=";
                case TokenType::Punctuator::ShiftEqual_R :
                    return ">>=";
                case TokenType::Punctuator::Perce :
                    return "%";
                case TokenType::Punctuator::PerceEqual :
                    return "%=";
                case TokenType::Punctuator::Shift_L :
                    return "<<";
                case TokenType::Punctuator::ShiftEqual_L :
                    return "<<=";
                case TokenType::Punctuator::Less :
                    return "<";
                case TokenType::Punctuator::LessEqual :
                    return"<=";
            }
            return "";
        },
        [](TokenType::NewLine&){
            return "\n";
        },
        [](TokenType::WhiteSpace&){
            return " ";
        },
        [](TokenType::PPNumber& tok){
            return tok.sequence;
        },
        [](TokenType::Identifier& tok){
            return tok.sequence;
        },
        [](TokenType::CharacterConstant& tok){
            return tok.sequence;
        },
        [](TokenType::HeaderName& tok){
            return tok.sequence;
        },
        [](TokenType::StringLiteral& tok){
            return tok.sequence;
        },
        [](TokenType::IntegerConstant& tok){
            std::stringstream ss;
            std::visit(overloaded{
                [&](auto val){
                    ss << val;
                }
            }, tok);
            return ss.str();
        },
        [](TokenType::FloatingConstant& tok){
            std::stringstream ss;
            ss << tok;
            return ss.str();
        },
    }, *this);
}

int TokenType::PPNumber::base(){
    int base = 10;
    if(sequence.starts_with("0x") || sequence.starts_with("0X")){
        base = 16;
    }else if(sequence.starts_with("0")){
        base = 8;
    }
    return base;
}

template<>
intmax_t TokenType::PPNumber::get<intmax_t>(){
    return std::strtoimax(sequence.c_str(), nullptr, base());
}

template<>
uintmax_t TokenType::PPNumber::get<uintmax_t>(){
    return std::strtoumax(sequence.c_str(), nullptr, base());
}

template<>
double TokenType::PPNumber::get<double>(){
    return std::stod(sequence);
}

template<>
long double TokenType::PPNumber::get<long double>(){
    return std::stold(sequence);
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

TokenType::StringLiteral::StringLiteral(std::string sequence) : sequence(sequence){
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

TokenType::Keyword::Keyword(std::string val) : value(val){
    if(!is_keyword(value)){
        throw Exception::Exception("unknown keyword '" + value + "'");
    }
}

bool TokenType::Keyword::is_keyword(std::string val){
    static const std::unordered_set<std::string> keywords {
        "auto", "extern", "short", "while", "break", "float", "signed", "case", "for", "sizeof",
        "char", "goto", "static", "const", "if", "struct", "continue", "inline", "switch", "default",
        "int", "typedef", "_Generic", "do", "long", "union", "_Imaginary", "double", "register", "unsigned",
        "_Noreturn", "else", "restrict", "void", "_Static_assert", "enum", "return", "volatile", "_Thread_local", "_Alignas",
        "_Alignaof", "_Atomic", "_Bool", "_Complex"
    };
    return keywords.contains(val);
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