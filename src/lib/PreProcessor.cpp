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

#include <PreProcessor.hpp>

#include <cctype>
#include <string>
#include <regex>
#include <Error.hpp>

using namespace WasmVM;

PreProcessor::PreProcessor(std::filesystem::path path){
    streams.emplace(path);
}

std::optional<Token> PreProcessor::get(){
    std::optional<Token> token = streams.top().get();
    // TODO:
    return token;
}

PreProcessor::TokenStream::TokenStream(std::filesystem::path path) :
    source(path)
{}

#define hex_prefix_re "0[xX]"
#define hex_digit_re "[0-9a-fA-F]"
#define digit_re "[0-9]"
#define nonzero_digit_re "[1-9]"
#define bin_exp_re "[pP][\\+\\-]?[0-9]+"
#define exp_re "[eE][\\+\\-]?[0-9]+"
#define octal_re "0[0-7]*"

std::optional<Token> PreProcessor::TokenStream::get(){
    auto ch = source.get();
    switch(ch){
        case ' ' :
        case '\t' :
        case '\v' :
        case '\f' :{
            auto wh = source.get();
            while(wh == ' ' || wh == '\t' || wh == '\v' || wh == '\f'){
                wh = source.get();
            }
            source.unget();
            return Token(TokenType::WhiteSpace(), source.position());
        }break;
        case '[' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_L), source.position());
        break;
        case ']' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_R), source.position());
        break;
        case '(' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Paren_L), source.position());
        break;
        case ')' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Paren_R), source.position());
        break;
        case '{' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_L), source.position());
        break;
        case '}' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_R), source.position());
        break;
        case ';' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Semi), source.position());
        break;
        case '~' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Tlide), source.position());
        break;
        case '?' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Query), source.position());
        break;
        case ',' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Comma), source.position());
        break;
        case '#':
            if(source.peek() == '#'){
                auto pos = source.position();
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleHash), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Hash), source.position());
            }
        break;
        case '.':{
            auto pos = source.position();
            auto next = source.get();
            if(next == '.' && source.peek() == '.'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Ellipsis), pos);
            }else{
                source.unget();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Dot), source.position());
            }
        }break;
        case '*':
            if(source.peek() == '='){
                auto pos = source.position();
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::AsterEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Aster), source.position());
            }
        break;
        case '^':
            if(source.peek() == '='){
                auto pos = source.position();
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::CaretEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Caret), source.position());
            }
        break;
        case '!':
            if(source.peek() == '='){
                auto pos = source.position();
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::ExclamEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Exclam), source.position());
            }
        break;
        case '=':
            if(source.peek() == '='){
                auto pos = source.position();
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Equal), source.position());
            }
        break;
        case '/':
            if(source.peek() == '='){
                auto pos = source.position();
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::SlashEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Slash), source.position());
            }
        break;
        case ':':
            if(source.peek() == '>'){
                auto pos = source.position();
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_R), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Colon), source.position());
            }
        break;
        case '+':{
            auto pos = source.position();
            if(source.peek() == '+'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoublePlus), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::PlusEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Plus), source.position());
            }
        }break;
        case '&':{
            auto pos = source.position();
            if(source.peek() == '&'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleAmp), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::AmpEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Amp), source.position());
            }
        }break;
        case '|':{
            auto pos = source.position();
            if(source.peek() == '|'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleBar), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::BarEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bar), source.position());
            }
        }break;
        case '-':{
            auto pos = source.position();
            if(source.peek() == '-'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleMinus), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::MinusEqual), pos);
            }else if(source.peek() == '>'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Arrow), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Minus), source.position());
            }
        }break;
        case '>':{
            auto pos = source.position();
            if(source.peek() == '>'){
                source.get();
                if(source.peek() == '='){
                    source.get();
                    return Token(TokenType::Punctuator(TokenType::Punctuator::ShiftEqual_R), pos);
                }else{
                    return Token(TokenType::Punctuator(TokenType::Punctuator::Shift_R), pos);
                }
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::GreatEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Great), source.position());
            }
        }break;
        case '<':{
            auto pos = source.position();
            if(source.peek() == '<'){
                source.get();
                if(source.peek() == '='){
                    source.get();
                    return Token(TokenType::Punctuator(TokenType::Punctuator::ShiftEqual_L), pos);
                }else{
                    return Token(TokenType::Punctuator(TokenType::Punctuator::Shift_L), pos);
                }
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::LessEqual), pos);
            }else if(source.peek() == ':'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_L), pos);
            }else if(source.peek() == '%'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_L), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Less), source.position());
            }
        }break;
        case '%':{
            auto pos = source.position();
            if(source.peek() == ':'){
                source.get();
                if(source.peek() == '%'){
                    source.get();
                    if(source.peek() == ':'){
                        source.get();
                        return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleHash), pos);
                    }else{
                        source.unget();
                    }
                }
                return Token(TokenType::Punctuator(TokenType::Punctuator::Hash), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::PerceEqual), pos);
            }else if(source.peek() == '>'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_R), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Perce), source.position());
            }
        }break;
        case '\n':
            return Token(TokenType::NewLine(),source.position());
        break;
        default:
        break;
    }
    if(std::isdigit(ch)){
        auto pos = source.position();
        // pp-number
        int base = 10;
        std::string sequence;
        // digits
        if(ch == '0'){
            sequence += ch;
            auto next = source.peek();
            if(next == 'x' || next == 'X'){
                // Base 16
                sequence += source.get();
                base = 16;
            }else{
                // Base 8
                base = 8;
            }
        }
        while(std::isxdigit(ch)){
            sequence += ch;
            ch = source.get();
        }
        if(sequence.empty() && ch != '.'){
            throw Exception::Error(source.position(), "expected digits in pp-number");
        }
        // fraction
        if(ch == '.'){
            sequence += ch;
            while(std::isxdigit(ch = source.get())){
                sequence += ch;
            }
        }
        // exponent
        if((base == 10 && (ch == 'e' || ch == 'E'))
            || (base == 16 && (ch == 'p' || ch == 'P'))
        ){
            sequence += ch;
            auto sign = source.peek();
            if(sign == '-' || sign == '+'){
                sequence += source.get();
            }
            while(std::isdigit(ch = source.get())){
                sequence += ch;
            }
        }
        // suffix
        switch(ch){
            case 'u':
            case 'U':
                sequence += ch;
                ch = source.peek();
                if(ch == 'l' || ch == 'L'){
                    sequence += source.get();
                    ch = source.peek();
                    if(ch == 'l' || ch == 'L'){
                        sequence += source.get();
                    }
                }
            break;
            case 'f':
            case 'F':
                sequence += ch;
            break;
            case 'l':
            case 'L':
                sequence += ch;
                ch = source.peek();
                if(ch == 'l' || ch == 'L'){
                    sequence += source.get();
                    ch = source.peek();
                }
                if(ch == 'u' || ch == 'U'){
                    sequence += source.get();
                }
            break;
            default:
                source.unget();
            break;
        }
        static const std::regex int_re(
            "(" "(" hex_prefix_re hex_digit_re "+" ")" "|" "(" octal_re  ")" "|" "(" nonzero_digit_re digit_re "*" ")" ")"
            "(" "([uU]((ll?)|(LL?))?)" "|" "(((ll?)|(LL?))[uU]?)" ")?"
        );
        static const std::regex float_re(
            "(" "(" hex_prefix_re 
                    "(" "(" hex_digit_re "*" "\\." hex_digit_re "+" ")" "|" "(" hex_digit_re "+" "\\." ")" ")"
                    "(" bin_exp_re ")?"
                ")" "|" 
                "(" hex_digit_re "+" bin_exp_re ")"
            ")" "|"
            "(" "(" 
                    "(" "(" digit_re "*" "\\." digit_re "+" ")" "|" "(" digit_re "+" "\\." ")" ")"
                    "(" exp_re ")?"
                ")" "|"
                "(" digit_re "+" exp_re ")"
            ")"
            "[fFlL]?"
        );
        std::smatch m;
        if(!std::regex_match(sequence, m, int_re) && !std::regex_match(sequence, m, float_re)){
            throw Exception::Error(pos, "pp-number must be integer or floating-point constant");
        }
        return Token(TokenType::PPNumber(sequence), pos);
    }else if(std::isalpha(ch) || ch == '_'){
        // TODO: identifier
    }
    return std::nullopt;
}