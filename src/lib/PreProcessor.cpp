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

using namespace WasmVM;

PreProcessor::PreProcessor(std::filesystem::path path){
    streams.emplace(new SourceStream<std::ifstream>(path, path));
}
PreProcessor::PreProcessor(std::filesystem::path path, std::string source){
    streams.emplace(new SourceStream<std::istringstream>(path, source));
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

static std::string trim(std::string str){
    str.erase(str.find_last_not_of(" \t\v\f") + 1);
    str.erase(0, str.find_first_not_of(" \t\v\f"));
    return str;
}

PreProcessor::Line::iterator PreProcessor::skip_whitespace(PreProcessor::Line::iterator it, PreProcessor::Line::iterator end){
    while(it != end && it->hold<TokenType::WhiteSpace>()){
        it = std::next(it);
    }
    return it;
}

bool PreProcessor::readline(){
    // Reset line
    line.clear();
    line.type = Line::None;
    // Get line tokens
    if(streams.empty()){
        return false;
    }
    Scanner scanner(streams.top().get());
    while(line.empty() && !streams.empty()){
        for(PPToken token = scanner.get(); !token.hold<TokenType::NewLine>(); token = scanner.get()){
            if(!token.has_value()){
                streams.pop();
                if(!streams.empty()){
                    scanner = Scanner(streams.top().get());
                }
                break;
            }
            line.emplace_back(token);
        }
    }
    // Trim whitespaces
    line.erase(line.begin(), skip_whitespace(line.begin(), line.end()));
    while(!line.empty() && line.back().hold<TokenType::WhiteSpace>()){
        line.pop_back();
    }
    // Set type
    if(line.empty()){
        return false;
    }else if(line.front().hold<TokenType::Punctuator>() && line.front().value() == TokenType::Punctuator(TokenType::Punctuator::Hash)){
        line.type = Line::Directive;
    }else{
        line.type = Line::Text;
    }
    return true;
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
        std::smatch m;
        if(!std::regex_match(sequence, m, int_re) && !std::regex_match(sequence, m, float_re)){
            throw Exception::Error(pos, "pp-number must be integer or floating-point constant");
        }
        return Token(TokenType::PPNumber(sequence), pos);
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

bool PreProcessor::Macro::operator==(const Macro& op) const {
    if((op.name != name) || (op.params.has_value() != params.has_value()) || (op.replacement.size() != replacement.size())){
        return false;
    }
    for(auto op_it = op.replacement.begin(), it = replacement.begin(); op_it != op.replacement.end(); op_it = std::next(op_it)){
        if(op_it->has_value() != it->has_value()){
            return false;
        }
        if(op_it->has_value() && (op_it->value() != it->value())){
            return false;
        }
    }
    if(op.params.has_value()){
        for(auto op_it = op.params->begin(), it = params->begin(); op_it != op.params->end(); op_it = std::next(op_it)){
            if(*op_it != *it){
                return false;
            }
        }
    }
    return true;
}

// void PreProcessor::error_directive(PPToken& token){
//     auto pos = token.value().pos;
//     token = streams.top().get();
//     skip_whitespace(token, streams.top());
//     if(token && std::holds_alternative<TokenType::StringLiteral>(token.value())
//         && std::holds_alternative<std::string>(((TokenType::StringLiteral)token.value()).value)){
//         throw Exception::Error(pos, std::get<std::string>(((TokenType::StringLiteral)token.value()).value));
//     }else{
//         throw Exception::Error(pos, "");
//     }
// }

void PreProcessor::define_directive(){
    auto cur = skip_whitespace(line.begin(), line.end());
    auto pos = cur->value().pos;
    Macro macro;
    // name
    if(!cur->hold<TokenType::Identifier>()){
        throw Exception::Error(pos, "expected identifier for macro name");
    }
    macro.name = ((TokenType::Identifier)cur->value()).sequence;
    cur = std::next(cur);
    if(cur != line.end()){
        // parameters
        if(cur->value() == Token(TokenType::Punctuator(TokenType::Punctuator::Paren_L))){
            macro.params.emplace();
            cur = std::next(cur);
            while(cur != line.end() && cur->has_value()){
                if(cur->hold<TokenType::Identifier>()){
                    std::string param = ((TokenType::Identifier)cur->value()).sequence;
                    if(param == "__VA_ARGS__"){
                        throw Exception::Error(cur->value().pos, "__VA_ARGS__ is reserved for variable arguments");
                    }else if(std::find(macro.params->begin(), macro.params->end(), param) != macro.params->end()){
                        throw Exception::Error(cur->value().pos, "duplicated macro parameter name");
                    }
                    macro.params.value().emplace_back(param);
                }else if(cur->hold<TokenType::Punctuator>()){
                    auto punct = ((TokenType::Punctuator)cur->value()).type;
                    if(punct == TokenType::Punctuator::Ellipsis){
                        macro.params.value().emplace_back("...");
                    }else if(punct == TokenType::Punctuator::Paren_R){
                        break;
                    }else if(punct != TokenType::Punctuator::Comma){
                        throw Exception::Error(cur->value().pos, "unexpected token in macro definition");
                    }
                }else if(!cur->hold<TokenType::WhiteSpace>()){
                    throw Exception::Error(cur->value().pos, "unexpected token in macro definition");
                }
                cur = std::next(cur);
            }
            if(cur == line.end() || !cur->has_value()){
                throw Exception::Error(pos, "unclosed parameter list in macro definition");
            }
            cur = std::next(cur);
        }
        // replacements
        cur = skip_whitespace(cur, line.end());
        macro.replacement.assign(cur, line.end());
    }

    // Check redifinition
    if((macros.contains(macro.name)) && (macros[macro.name] != macro)){
        Exception::Warning("existing macro redefined");
    }

    // Insert macro
    macros[macro.name] = macro;
}

PreProcessor::PPToken PreProcessor::get(){
    // Fill line
    if(line.empty()){
        while(readline() && line.type == Line::Directive){
            Line::iterator cur = skip_whitespace(std::next(line.begin()), line.end());
            if(cur != line.end() && cur->hold<TokenType::Identifier>()){
                std::string direcitve_name = ((TokenType::Identifier)cur->value()).sequence;
                line.erase(line.begin(), std::next(cur));
                if(direcitve_name == "if"){
                    // TODO:
                }else if(direcitve_name == "ifdef"){
                    // TODO:
                }else if(direcitve_name == "ifndef"){
                    // TODO:
                }else if(direcitve_name == "elif"){
                    // TODO:
                }else if(direcitve_name == "else"){
                    // TODO:
                }else if(direcitve_name == "endif"){
                    // TODO:
                }else if(direcitve_name == "define"){
                    define_directive();
                }else if(direcitve_name == "undef"){
                    // TODO:
                }else if(direcitve_name == "line"){
                    // TODO:
                }else if(direcitve_name == "error"){
                    // TODO:
                }else if(direcitve_name == "pragma"){
                    // TODO:
                }
            }
        }
        if(line.type == Line::None){
            return std::nullopt;
        }
    }
    replace_macro(line, macros);
    // Result
    PreProcessor::PPToken token = line.front();
    line.pop_front();
    return token;
}

void PreProcessor::replace_macro(Line& line, std::unordered_map<std::string, Macro> macro_map){
    for(auto cur = line.begin(); cur != line.end();){
        if(cur->hold<TokenType::Identifier>()){
            TokenType::Identifier identifier = cur->value();
            // Find macro
            if((!cur->expanded.contains(identifier.sequence)) && macro_map.contains(identifier.sequence)){
                Macro& macro = macro_map[identifier.sequence];
                auto macro_next = std::next(cur);
                // Construct macro map & arg map
                std::unordered_map<std::string, Line> args = {};
                // Get args
                if(macro.params){
                    if(macro_next == line.end()){
                        cur = std::next(cur);
                        continue;
                    }else if(macro_next->value() == Token(TokenType::Punctuator(TokenType::Punctuator::Paren_L))){
                        // Get args
                        if(macro.params->empty()){
                            macro_next = std::next(macro_next);
                        }else{
                            for(std::string param : macro.params.value()){
                                int nest_level = 0;
                                auto arg_end = skip_whitespace(std::next(macro_next), line.end());
                                macro_next = arg_end;
                                while(arg_end != line.end()){
                                    if((nest_level == 0) && (
                                        ((param != "...") && (arg_end->value() == Token(TokenType::Punctuator(TokenType::Punctuator::Comma)))) ||
                                        (arg_end->value() == Token(TokenType::Punctuator(TokenType::Punctuator::Paren_R)))
                                    )){
                                        break;
                                    }
                                    if(arg_end->value() == Token(TokenType::Punctuator(TokenType::Punctuator::Paren_L))){
                                        nest_level += 1;
                                    }else if(arg_end->value() == Token(TokenType::Punctuator(TokenType::Punctuator::Paren_R))){
                                        nest_level -= 1;
                                    }
                                    arg_end = std::next(arg_end);
                                }
                                if(arg_end == line.end()){
                                    throw Exception::Error(cur->value().pos, "expect ')' for function-like macro");
                                }
                                Line& arg_line = args[(param == "...") ? "__VA_ARGS__" : param];
                                arg_line.insert(arg_line.end(), macro_next, arg_end);
                                for(PPToken& tok : arg_line){
                                    tok.skipped = true;
                                }
                                replace_macro(arg_line, macro_map);
                                macro_next = arg_end;
                            }
                        }
                        macro_next = std::next(macro_next);
                    }
                }
                // Replacement list
                Line replaced_line;
                replaced_line.insert(replaced_line.end(), macro.replacement.begin(), macro.replacement.end());
                // Replace args and # operator
                for(auto replaced_cur = replaced_line.begin(); replaced_cur != replaced_line.end();){
                    if(replaced_cur->skipped){
                        replaced_cur->skipped = false;
                    }else if(replaced_cur->hold<TokenType::Identifier>()){
                        std::string seq = ((TokenType::Identifier)replaced_cur->value()).sequence;
                        if(args.contains(seq)){
                            Line& arg = args[seq];
                            Line::iterator next = std::next(replaced_cur);
                            replaced_line.erase(replaced_cur);
                            if(arg.empty()){
                                replaced_line.emplace(next);
                                replaced_cur = next;
                            }else{
                                replaced_cur = replaced_line.insert(next, arg.begin(), arg.end());
                            }
                            continue;
                        }
                    }else if(replaced_cur->hold<TokenType::Punctuator>() && (replaced_cur->value() == Token(TokenType::Punctuator(TokenType::Punctuator::Hash)))){
                        auto param = skip_whitespace(std::next(replaced_cur), replaced_line.end());
                        if((param != replaced_line.end()) && param->hold<TokenType::Identifier>() && args.contains(((TokenType::Identifier)param->value()).sequence)){
                            Line& arg = args[((TokenType::Identifier)param->value()).sequence];
                            std::string literal = "";
                            if(!arg.empty()){
                                std::stringstream ss;
                                for(PPToken& tok : arg){
                                    if(!tok.hold<TokenType::WhiteSpace>() && !tok.hold<TokenType::NewLine>()){
                                        ss << tok.value() << " ";
                                    }
                                }
                                literal = std::regex_replace(trim(ss.str()), std::regex("\\\\"), "\\");
                                literal = std::regex_replace(literal, std::regex("\""), "\\\"");
                            }
                            auto new_cur = replaced_line.insert(replaced_cur, Token(TokenType::StringLiteral("\"" + literal + "\""), param->value().pos));
                            replaced_line.erase(replaced_cur, std::next(param));
                            replaced_cur = new_cur;
                        }
                    }
                    replaced_cur = std::next(replaced_cur);
                }
                // ## operator
                for(auto replaced_cur = replaced_line.begin(); replaced_cur != replaced_line.end();){
                    if(replaced_cur->hold<TokenType::Punctuator>() && !replaced_cur->skipped && (replaced_cur->value() == Token(TokenType::Punctuator(TokenType::Punctuator::DoubleHash)))){
                        // Get next & prev
                        auto next = skip_whitespace(std::next(replaced_cur), replaced_line.end());
                        if((replaced_cur == skip_whitespace(replaced_line.begin(), replaced_line.end())) || (next == replaced_line.end())){
                            throw Exception::Error(replaced_cur->value().pos, "'##' shall not occur at the beginning or at the end of a macro replacement list");
                        }
                        auto prev = std::prev(replaced_cur);
                        while(prev != replaced_line.begin() && prev->hold<TokenType::WhiteSpace>()){
                            prev = std::prev(prev);
                        }
                        // Concate token
                        PPToken concated_token;
                        concated_token.skipped = true;
                        if(prev->has_value() || next->has_value()){
                            SourceStream<std::stringstream> str_source(replaced_cur->value().pos.path);
                            if(prev->has_value()){
                                str_source << prev->value();
                            }
                            if(next->has_value()){
                                str_source << next->value();
                            }
                            // Rescan token 
                            Scanner scanner(&str_source);
                            concated_token = scanner.get();
                            concated_token->pos = prev->has_value() ? prev->value().pos : next->value().pos;
                            if(scanner.get().has_value()){
                                throw Exception::Error(concated_token->pos, "'##' operator formed an invalid preprocessing token");
                            }
                        }
                        replaced_cur = std::next(next);
                        replaced_line.erase(prev, replaced_cur);
                        replaced_line.insert(replaced_cur, concated_token);
                    }else{
                        replaced_cur = std::next(replaced_cur);
                    }
                }
                // Further replacement
                std::unordered_map<std::string, Macro> new_map = macro_map;
                new_map.erase(macro.name);
                replace_macro(replaced_line, new_map);
                // Join line
                replaced_line.remove_if([](PPToken& tok){
                    return !tok.has_value();
                });
                line.erase(cur, macro_next);
                cur = line.insert(macro_next, replaced_line.begin(), replaced_line.end());
                continue;
            }
        }
        cur = std::next(cur);
    }
}
