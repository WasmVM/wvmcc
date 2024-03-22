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
#include <fstream>
#include <sstream>
#include <regex>
#include <list>
#include <Error.hpp>
#include <cstdint>
#include <variant>

using namespace WasmVM;

static std::string trim(std::string str){
    str.erase(str.find_last_not_of(" \t\v\f") + 1);
    str.erase(0, str.find_first_not_of(" \t\v\f"));
    return str;
}

PreProcessor::Scanner::Scanner(Stream* stream) : stream(stream), state(LineState::unknown){
}

#define hex_prefix_re "0[xX]"
#define hex_digit_re "[0-9a-fA-F]"
#define digit_re "[0-9]"
#define nonzero_digit_re "[1-9]"
#define bin_exp_re "[pP][\\+\\-]?[0-9]+"
#define exp_re "[eE][\\+\\-]?[0-9]+"
#define octal_re "0[0-7]*"

template<typename IntType, typename CharType> requires std::integral<IntType> && std::integral<CharType>
static void escape_sequence(IntType& ch, SourceStreamBase<IntType, CharType>* stream, std::string& sequence, SourcePos& pos){
    // escape
    ch = stream->get();
    switch(ch){
        case '\'':
        case '"':
        case '?':
        case '\\':
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 't':
        case 'v':
        case 'r':
            sequence += ch;
            ch = stream->get();
        break;
        case 'x':
            // hexadecimal
            sequence += ch;
            while(std::isxdigit(ch = stream->get())){
                sequence += ch;
            }
        break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            // octal
            for(int i = 0; (i < 3) && (ch >= '0') && (ch <= '7'); ++i){
                sequence += ch;
                ch = stream->get();
            }
        break;
        case 'u':
            // universal character name
            sequence += ch;
            ch = stream->get();
            for(int i = 0; i < 4; ++i){
                if(!std::isxdigit(ch)){
                    throw Exception::Error(pos, std::string("invalid universal character name"));
                }
                sequence += ch;
                ch = stream->get();
            }
        break;
        case 'U':
            // universal character name
            sequence += ch;
            ch = stream->get();
            for(int i = 0; i < 8; ++i){
                if(!std::isxdigit(ch)){
                    throw Exception::Error(pos, std::string("invalid universal character name"));
                }
                sequence += ch;
                ch = stream->get();
            }
        break;
        default:
            throw Exception::Error(pos, std::string("unknown escape sequence '\\") + (char)(ch) + "'");
        break;
    }
}

template<typename IntType, typename Traits> requires std::integral<IntType>
static bool is_source_character(IntType& ch){
    if(std::isalnum(ch)){
        return true;
    }
    switch(ch){
        case '!':
        case '"':
        case '#':
        case '%':
        case '&':
        case '\'':
        case '(':    
        case ')':
        case '*':
        case '+':
        case ',':
        case '-':
        case '.':
        case '/':
        case ':':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '_':
        case '{':
        case '|':
        case '}':
        case '~':
        case ' ':
        case '\t':
        case '\v':
        case '\f':
        case '\n':
        case Traits::eof():
            return true;
        break;
        default:
            return false;
        break;
    }
}

PreProcessor::PPToken PreProcessor::Scanner::get(){
    auto ch = stream->get();
    SourcePos pos = stream->pos;
    // whitespace
    if(ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f'){
        auto wh = stream->get();
        while(wh == ' ' || wh == '\t' || wh == '\v' || wh == '\f'){
            wh = stream->get();
        }
        stream->unget();
        return Token(TokenType::WhiteSpace(), pos);
    }
    // single line comment
    if(ch == '/' && stream->peek() == '/'){
        while(ch != '\n'){
            ch = stream->get();
        }
        stream->unget();
        return Token(TokenType::WhiteSpace(), pos);
    }
    // multiple lines comment
    if(ch == '/' && stream->peek() == '*'){
        while(!(ch == '*' && stream->peek() == '/')){
            ch = stream->get();
        }
        stream->get();
        return Token(TokenType::WhiteSpace(), pos);
    }
    // newline
    if(ch == '\r'){
        if(stream->peek() != '\n'){
            throw Exception::Error(pos, std::string("invalid source character \\r"));
        }else{
            ch = stream->get();
        }
    }
    if(ch == '\n'){
        state = LineState::unknown;
        return Token(TokenType::NewLine(), pos);
    }
    // header name
    if(state == LineState::include){
        state = LineState::normal;
        if(ch == '\"'){
            std::string sequence;
            sequence += ch;
            ch = stream->get();
            while(ch != '\"' && ch != '\n' && ch != StreamType::traits_type::eof() && is_source_character<StreamType::int_type, StreamType::traits_type>(ch)){
                sequence += ch;
                ch = stream->get();
            }
            if(ch != '\"'){
                throw Exception::Error(pos, "unclosed header name");
            }else{
                sequence += ch;
            }
            return Token(TokenType::HeaderName(sequence), pos);
        }else if(ch == '<'){
            std::string sequence;
            sequence += ch;
            ch = stream->get();
            while(ch != '>' && ch != '\n' && ch != StreamType::traits_type::eof() && is_source_character<StreamType::int_type, StreamType::traits_type>(ch)){
                sequence += ch;
                ch = stream->get();
            }
            if(ch != '>'){
                throw Exception::Error(pos, "unclosed header name");
            }else{
                sequence += ch;
            }
            return Token(TokenType::HeaderName(sequence), pos);
        }
    }
    // string literal
    if((ch == '\"') ||
        ((ch == 'L' || ch == 'U') && (stream->peek() == '\"')) ||
        (ch == 'u' && (stream->peek() == '\"' || stream->peek() == '8'))
    ){
        state = LineState::normal;
        std::string sequence;
        sequence += ch;
        if(ch != '\"'){
            if(ch == 'u' && stream->peek() == '8'){
                sequence += stream->get();
            }
            ch = stream->get();
            sequence += ch;
        }
        ch = stream->get();
        while(ch != '\"'){
            sequence += ch;
            if(ch == '\\'){
                escape_sequence(ch, stream, sequence, pos);
            }else{
                ch = stream->get();
            }
            if(ch == StreamType::traits_type::eof() || ch == '\n'){
                throw Exception::Error(pos, "unclosed string literal");
            }
        }
        sequence += ch;
        return Token(TokenType::StringLiteral(sequence), pos);
    }
    // Character constant
    if((ch == '\'') || ((ch == 'L' || ch == 'u' || ch == 'U') && (stream->peek() == '\''))){
        state = LineState::normal;
        std::string sequence;
        sequence += ch;
        if(ch != '\''){
            ch = stream->get();
            sequence += ch;
        }
        ch = stream->get();
        while(ch != '\''){
            sequence += ch;
            if(ch == '\\'){
                escape_sequence(ch, stream, sequence, pos);
            }else{
                ch = stream->get();
            }
            if(ch == StreamType::traits_type::eof() || ch == '\n'){
                throw Exception::Error(pos, "unclosed character constant");
            }
        }
        sequence += ch;
        return Token(TokenType::CharacterConstant(sequence), pos);
    }
    // Punctuator
    switch(ch){
        case '[' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_L), pos);
        break;
        case ']' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_R), pos);
        break;
        case '(' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Paren_L), pos);
        break;
        case ')' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Paren_R), pos);
        break;
        case '{' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_L), pos);
        break;
        case '}' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_R), pos);
        break;
        case ';' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Semi), pos);
        break;
        case '~' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Tlide), pos);
        break;
        case '?' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Query), pos);
        break;
        case ',' :
            state = LineState::normal;
            return Token(TokenType::Punctuator(TokenType::Punctuator::Comma), pos);
        break;
        case '#':
            if(stream->peek() == '#'){
                state = LineState::normal;
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleHash), pos);
            }else{
                if(state == LineState::unknown){
                    state = LineState::hashed;
                }else{
                    state = LineState::normal;
                }
                return Token(TokenType::Punctuator(TokenType::Punctuator::Hash), pos);
            }
        break;
        case '.':{
            state = LineState::normal;
            if(std::isdigit(stream->peek())){
                break;
            }
            auto next = stream->get();
            if(next == '.' && stream->peek() == '.'){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Ellipsis), pos);
            }else{
                stream->unget();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Dot), pos);
            }
        }break;
        case '*':
            state = LineState::normal;
            if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::AsterEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Aster), pos);
            }
        break;
        case '^':
            state = LineState::normal;
            if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::CaretEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Caret), pos);
            }
        break;
        case '!':
            state = LineState::normal;
            if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::ExclamEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Exclam), pos);
            }
        break;
        case '=':
            state = LineState::normal;
            if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Equal), pos);
            }
        break;
        case '/':
            state = LineState::normal;
            if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::SlashEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Slash), pos);
            }
        break;
        case ':':
            if((state != LineState::hashed) && (stream->peek() == '>')){
                state = LineState::normal;
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_R), pos);
            }else{
                if((state != LineState::hashed) || (stream->peek() != '%')){
                    state = LineState::normal;
                }
                return Token(TokenType::Punctuator(TokenType::Punctuator::Colon), pos);
            }
        break;
        case '+':{
            state = LineState::normal;
            if(stream->peek() == '+'){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoublePlus), pos);
            }else if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::PlusEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Plus), pos);
            }
        }break;
        case '&':{
            state = LineState::normal;
            if(stream->peek() == '&'){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleAmp), pos);
            }else if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::AmpEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Amp), pos);
            }
        }break;
        case '|':{
            state = LineState::normal;
            if(stream->peek() == '|'){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleBar), pos);
            }else if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::BarEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bar), pos);
            }
        }break;
        case '-':{
            state = LineState::normal;
            if(stream->peek() == '-'){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleMinus), pos);
            }else if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::MinusEqual), pos);
            }else if(stream->peek() == '>'){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Arrow), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Minus), pos);
            }
        }break;
        case '>':{
            state = LineState::normal;
            if(stream->peek() == '>'){
                stream->get();
                if(stream->peek() == '='){
                    stream->get();
                    return Token(TokenType::Punctuator(TokenType::Punctuator::ShiftEqual_R), pos);
                }else{
                    return Token(TokenType::Punctuator(TokenType::Punctuator::Shift_R), pos);
                }
            }else if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::GreatEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Great), pos);
            }
        }break;
        case '<':{
            if((state != LineState::hashed) && (stream->peek() == ':')){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_L), pos);
            }else if((state != LineState::hashed) && (stream->peek() == '%')){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_L), pos);
            }
            state = LineState::normal;
            if(stream->peek() == '<'){
                stream->get();
                if(stream->peek() == '='){
                    stream->get();
                    return Token(TokenType::Punctuator(TokenType::Punctuator::ShiftEqual_L), pos);
                }else{
                    return Token(TokenType::Punctuator(TokenType::Punctuator::Shift_L), pos);
                }
            }else if(stream->peek() == '='){
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::LessEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Less), pos);
            }
        }break;
        case '%':{
            if((state != LineState::hashed) && (stream->peek() == ':')){
                stream->get();
                if(stream->peek() == '%'){
                    stream->get();
                    if(stream->peek() == ':'){
                        state = LineState::normal;
                        stream->get();
                        return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleHash), pos);
                    }else{
                        stream->unget();
                    }
                }
                if(state == LineState::unknown){
                    state = LineState::hashed;
                }else{
                    state = LineState::normal;
                }
                return Token(TokenType::Punctuator(TokenType::Punctuator::Hash), pos);
            }else if(stream->peek() == '='){
                state = LineState::normal;
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::PerceEqual), pos);
            }else if((state != LineState::hashed) && (stream->peek() == '>')){
                state = LineState::normal;
                stream->get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_R), pos);
            }else{
                if((state != LineState::hashed) || (stream->peek() != ':')){
                    state = LineState::normal;
                }
                return Token(TokenType::Punctuator(TokenType::Punctuator::Perce), pos);
            }
        }break;
        default:
        break;
    }
    if(std::isdigit(ch) || ch == '.'){ // pp-number
        state = LineState::normal;
        int base = 10;
        std::string sequence;
        // digits
        if(ch == '0'){
            sequence += ch;
            auto next = stream->peek();
            if(next == 'x' || next == 'X'){
                // Base 16
                sequence += stream->get();
                base = 16;
            }
            ch = stream->get();
        }
        if(base == 16) {
            while(std::isxdigit(ch)){
                sequence += ch;
                ch = stream->get();
            }
        }else{
            while(std::isdigit(ch)){
                sequence += ch;
                ch = stream->get();
            }
        }
        if(sequence.empty() && ch != '.'){
            throw Exception::Error(stream->pos, "expected digits in pp-number");
        }
        // fraction
        if(ch == '.'){
            sequence += ch;
            if(base == 16) {
                while(std::isxdigit(ch = stream->get())){
                    sequence += ch;
                }
            }else{
                while(std::isdigit(ch = stream->get())){
                    sequence += ch;
                }
            }
        }
        // exponent
        if((ch == 'e' || ch == 'E')
            || (base == 16 && (ch == 'p' || ch == 'P'))
        ){
            sequence += ch;
            auto sign = stream->peek();
            if(sign == '-' || sign == '+'){
                sequence += stream->get();
            }
            while(std::isdigit(ch = stream->get())){
                sequence += ch;
            }
        }
        // suffix
        switch(ch){
            case 'u':
            case 'U':
                sequence += ch;
                ch = stream->peek();
                if(ch == 'l' || ch == 'L'){
                    sequence += stream->get();
                    ch = stream->peek();
                    if(ch == 'l' || ch == 'L'){
                        sequence += stream->get();
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
                ch = stream->peek();
                if(ch == 'l' || ch == 'L'){
                    sequence += stream->get();
                    ch = stream->peek();
                }
                if(ch == 'u' || ch == 'U'){
                    sequence += stream->get();
                }
            break;
            default:
                stream->unget();
            break;
        }
        static const std::regex int_re(
            "(" "(" hex_prefix_re hex_digit_re "+" ")" "|" "(" octal_re  ")" "|" "(" nonzero_digit_re digit_re "*" ")" ")"
            "(" "([uU]((ll?)|(LL?))?)" "|" "(((ll?)|(LL?))[uU]?)" ")?"
        );
        static const std::regex float_re(
            "(" hex_prefix_re
                "(" "(" hex_digit_re "*" "\\." hex_digit_re "+" ")" "|" "(" hex_digit_re "+" "\\.?" ")" ")" 
                bin_exp_re
            ")" "|"
            "(" "(" 
                    "(" "(" digit_re "*" "\\." digit_re "+" ")" "|" "(" digit_re "+" "\\." ")" ")"
                    "(" exp_re ")?"
                ")" "|"
                "(" digit_re "+" exp_re ")"
            ")"
            "[fFlL]?"
        );
        TokenType::PPNumber num(sequence);
        std::smatch m;
        if(std::regex_match(sequence, m, int_re)){
            num.type = TokenType::PPNumber::Int;
        }else if(std::regex_match(sequence, m, float_re)){
            num.type = TokenType::PPNumber::Float;
        }else{
            throw Exception::Error(pos, "pp-number must be integer or floating-point constant");
        }
        return Token(num, pos);
    }else if(std::isalpha(ch) || ch == '_' || ch == '\\'){ // identifier
        std::string sequence;
        while(std::isalnum(ch) || ch == '_' || ch == '\\'){
            if(ch == '\\'){
                ch = stream->get();
                if(ch == 'u' || ch == 'U'){
                    const int width = (ch == 'u') ? 4 : 8;
                    int val = 0;
                    for(int i = 0; i < width; ++i){
                        ch = stream->get();
                        if(!std::isxdigit(ch)){
                            throw Exception::Error(pos, std::string("invalid universal character name"));
                        }
                        int lower = std::tolower(ch);
                        val = (val << 4) + (std::isdigit(lower) ? (lower - '0') : (lower - 'a' + 10));
                    }
                    sequence += val;
                }else{
                    throw Exception::Error(pos, std::string("unexpected escape sequence in identifier"));
                }
            }else{
                sequence += ch;
            }
            ch = stream->get();
        }
        stream->unget();
        if((state == LineState::hashed) && (sequence == "include")){
            state = LineState::include;
        }else{
            state = LineState::normal;
        }
        return Token(TokenType::Identifier(sequence), pos);
    }else if(ch == StreamType::traits_type::eof()){ // EOF
        return std::nullopt;
    }else{
        throw Exception::Error(pos, std::string("invalid source character ") + (char)ch);
    }
}
