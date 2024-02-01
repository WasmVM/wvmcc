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
#include <sstream>
#include <regex>
#include <set>
#include <list>
#include <Error.hpp>

using namespace WasmVM;

PreProcessor::PreProcessor(std::filesystem::path path){
    streams.emplace(path);
}

PreProcessor::TokenStream::TokenStream(std::filesystem::path path) :
    source(path), state(LineState::unknown)
{}

#define hex_prefix_re "0[xX]"
#define hex_digit_re "[0-9a-fA-F]"
#define digit_re "[0-9]"
#define nonzero_digit_re "[1-9]"
#define bin_exp_re "[pP][\\+\\-]?[0-9]+"
#define exp_re "[eE][\\+\\-]?[0-9]+"
#define octal_re "0[0-7]*"

static void escape_sequence(SourceFile::int_type& ch, SourceFile& source, std::string& sequence, SourcePos& pos){
    // escape
    ch = source.get();
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
            ch = source.get();
        break;
        case 'x':
            // hexadecimal
            sequence += ch;
            while(std::isxdigit(ch = source.get())){
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
                ch = source.get();
            }
        break;
        case 'u':
            // universal character name
            sequence += ch;
            ch = source.get();
            for(int i = 0; i < 4; ++i){
                if(!std::isxdigit(ch)){
                    throw Exception::Error(pos, std::string("invalid universal character name"));
                }
                sequence += ch;
                ch = source.get();
            }
        break;
        case 'U':
            // universal character name
            sequence += ch;
            ch = source.get();
            for(int i = 0; i < 8; ++i){
                if(!std::isxdigit(ch)){
                    throw Exception::Error(pos, std::string("invalid universal character name"));
                }
                sequence += ch;
                ch = source.get();
            }
        break;
        default:
            throw Exception::Error(pos, std::string("unknown escape sequence '\\") + (char)(ch) + "'");
        break;
    }
}

static bool is_source_character(SourceFile::int_type& ch){
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
        case SourceFile::traits_type::eof():
            return true;
        break;
        default:
            return false;
        break;
    }
}

void PreProcessor::skip_whitespace(PPToken& token){
    while(token.hold<TokenType::WhiteSpace>()){
        token = streams.top().get();
    }
}

PreProcessor::PPToken PreProcessor::TokenStream::get(){
    if(!buffer.empty()){
        Token token = buffer.front();
        buffer.pop_front();
        return token;
    }
    auto ch = source.get();
    auto pos = source.position();
    // whitespace
    if(ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f'){
        auto wh = source.get();
        while(wh == ' ' || wh == '\t' || wh == '\v' || wh == '\f'){
            wh = source.get();
        }
        source.unget();
        return Token(TokenType::WhiteSpace(), pos);
    }
    // single line comment
    if(ch == '/' && source.peek() == '/'){
        while(ch != '\n'){
            ch = source.get();
        }
        source.unget();
        return Token(TokenType::WhiteSpace(), pos);
    }
    // multiple lines comment
    if(ch == '/' && source.peek() == '*'){
        while(!(ch == '*' && source.peek() == '/')){
            ch = source.get();
        }
        source.get();
        return Token(TokenType::WhiteSpace(), pos);
    }
    // newline
    if(ch == '\r'){
        if(source.peek() != '\n'){
            throw Exception::Error(pos, std::string("invalid source character \\r"));
        }else{
            ch = source.get();
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
            ch = source.get();
            while(ch != '\"' && ch != '\n' && ch != SourceFile::traits_type::eof() && is_source_character(ch)){
                sequence += ch;
                ch = source.get();
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
            ch = source.get();
            while(ch != '>' && ch != '\n' && ch != SourceFile::traits_type::eof() && is_source_character(ch)){
                sequence += ch;
                ch = source.get();
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
        ((ch == 'L' || ch == 'U') && (source.peek() == '\"')) ||
        (ch == 'u' && (source.peek() == '\"' || source.peek() == '8'))
    ){
        state = LineState::normal;
        std::string sequence;
        sequence += ch;
        if(ch != '\"'){
            if(ch == 'u' && source.peek() == '8'){
                sequence += source.get();
            }
            ch = source.get();
            sequence += ch;
        }
        ch = source.get();
        while(ch != '\"'){
            sequence += ch;
            if(ch == '\\'){
                escape_sequence(ch, source, sequence, pos);
            }else{
                ch = source.get();
            }
            if(ch == SourceFile::traits_type::eof() || ch == '\n'){
                throw Exception::Error(pos, "unclosed string literal");
            }
        }
        sequence += ch;
        return Token(TokenType::StringLiteral(sequence), pos);
    }
    // Character constant
    if((ch == '\'') || ((ch == 'L' || ch == 'u' || ch == 'U') && (source.peek() == '\''))){
        state = LineState::normal;
        std::string sequence;
        sequence += ch;
        if(ch != '\''){
            ch = source.get();
            sequence += ch;
        }
        ch = source.get();
        while(ch != '\''){
            sequence += ch;
            if(ch == '\\'){
                escape_sequence(ch, source, sequence, pos);
            }else{
                ch = source.get();
            }
            if(ch == SourceFile::traits_type::eof() || ch == '\n'){
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
            if(source.peek() == '#'){
                state = LineState::normal;
                source.get();
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
            if(std::isdigit(source.peek())){
                break;
            }
            auto next = source.get();
            if(next == '.' && source.peek() == '.'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Ellipsis), pos);
            }else{
                source.unget();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Dot), pos);
            }
        }break;
        case '*':
            state = LineState::normal;
            if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::AsterEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Aster), pos);
            }
        break;
        case '^':
            state = LineState::normal;
            if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::CaretEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Caret), pos);
            }
        break;
        case '!':
            state = LineState::normal;
            if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::ExclamEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Exclam), pos);
            }
        break;
        case '=':
            state = LineState::normal;
            if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Equal), pos);
            }
        break;
        case '/':
            state = LineState::normal;
            if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::SlashEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Slash), pos);
            }
        break;
        case ':':
            if((state != LineState::hashed) && (source.peek() == '>')){
                state = LineState::normal;
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_R), pos);
            }else{
                if((state != LineState::hashed) || (source.peek() != '%')){
                    state = LineState::normal;
                }
                return Token(TokenType::Punctuator(TokenType::Punctuator::Colon), pos);
            }
        break;
        case '+':{
            state = LineState::normal;
            if(source.peek() == '+'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoublePlus), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::PlusEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Plus), pos);
            }
        }break;
        case '&':{
            state = LineState::normal;
            if(source.peek() == '&'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleAmp), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::AmpEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Amp), pos);
            }
        }break;
        case '|':{
            state = LineState::normal;
            if(source.peek() == '|'){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleBar), pos);
            }else if(source.peek() == '='){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::BarEqual), pos);
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bar), pos);
            }
        }break;
        case '-':{
            state = LineState::normal;
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
                return Token(TokenType::Punctuator(TokenType::Punctuator::Minus), pos);
            }
        }break;
        case '>':{
            state = LineState::normal;
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
                return Token(TokenType::Punctuator(TokenType::Punctuator::Great), pos);
            }
        }break;
        case '<':{
            if((state != LineState::hashed) && (source.peek() == ':')){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_L), pos);
            }else if((state != LineState::hashed) && (source.peek() == '%')){
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_L), pos);
            }
            state = LineState::normal;
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
            }else{
                return Token(TokenType::Punctuator(TokenType::Punctuator::Less), pos);
            }
        }break;
        case '%':{
            if((state != LineState::hashed) && (source.peek() == ':')){
                source.get();
                if(source.peek() == '%'){
                    source.get();
                    if(source.peek() == ':'){
                        state = LineState::normal;
                        source.get();
                        return Token(TokenType::Punctuator(TokenType::Punctuator::DoubleHash), pos);
                    }else{
                        source.unget();
                    }
                }
                if(state == LineState::unknown){
                    state = LineState::hashed;
                }else{
                    state = LineState::normal;
                }
                return Token(TokenType::Punctuator(TokenType::Punctuator::Hash), pos);
            }else if(source.peek() == '='){
                state = LineState::normal;
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::PerceEqual), pos);
            }else if((state != LineState::hashed) && (source.peek() == '>')){
                state = LineState::normal;
                source.get();
                return Token(TokenType::Punctuator(TokenType::Punctuator::Brace_R), pos);
            }else{
                if((state != LineState::hashed) || (source.peek() != ':')){
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
            auto next = source.peek();
            if(next == 'x' || next == 'X'){
                // Base 16
                sequence += source.get();
                base = 16;
            }
            ch = source.get();
        }
        if(base == 16) {
            while(std::isxdigit(ch)){
                sequence += ch;
                ch = source.get();
            }
        }else{
            while(std::isdigit(ch)){
                sequence += ch;
                ch = source.get();
            }
        }
        if(sequence.empty() && ch != '.'){
            throw Exception::Error(source.position(), "expected digits in pp-number");
        }
        // fraction
        if(ch == '.'){
            sequence += ch;
            if(base == 16) {
                while(std::isxdigit(ch = source.get())){
                    sequence += ch;
                }
            }else{
                while(std::isdigit(ch = source.get())){
                    sequence += ch;
                }
            }
        }
        // exponent
        if((ch == 'e' || ch == 'E')
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
                ch = source.get();
                if(ch == 'u' || ch == 'U'){
                    const int width = (ch == 'u') ? 4 : 8;
                    int val = 0;
                    for(int i = 0; i < width; ++i){
                        ch = source.get();
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
            ch = source.get();
        }
        source.unget();
        if((state == LineState::hashed) && (sequence == "include")){
            state = LineState::include;
        }else{
            state = LineState::normal;
        }
        return Token(TokenType::Identifier(sequence), pos);
    }else if(ch == SourceFile::traits_type::eof()){ // EOF
        return std::nullopt;
    }else{
        throw Exception::Error(pos, std::string("invalid source character ") + (char)ch);
    }
}

PreProcessor::PPToken PreProcessor::LineStream::get(){
    // TODO:
    return std::nullopt;
}

bool PreProcessor::Macro::operator==(const Macro& op) const {
    if((op.name != name) || (op.params.size() != params.size()) || (op.replacement.size() != replacement.size())){
        return false;
    }
    for(auto op_it = op.params.begin(), it = params.begin(); op_it != op.params.end(); op_it = std::next(op_it)){
        if(*op_it != *it){
            return false;
        }
    }
    for(auto op_it = op.replacement.begin(), it = replacement.begin(); op_it != op.replacement.end(); op_it = std::next(op_it)){
        if(*op_it != *it){
            return false;
        }
    }
    return true;
}

void PreProcessor::error_directive(PPToken& token){
    auto pos = token.value().pos;
    token = streams.top().get();
    skip_whitespace(token);
    if(token && std::holds_alternative<TokenType::StringLiteral>(token.value())
        && std::holds_alternative<std::string>(((TokenType::StringLiteral)token.value()).value)){
        throw Exception::Error(pos, std::get<std::string>(((TokenType::StringLiteral)token.value()).value));
    }else{
        throw Exception::Error(pos, "");
    }
}

void PreProcessor::define_directive(PPToken& token){
    auto pos = token.value().pos;
    Macro macro;
    // name
    token = streams.top().get();
    skip_whitespace(token);
    if(!(token && std::holds_alternative<TokenType::Identifier>(token.value()))){
        throw Exception::Error(pos, "expected identifier for macro name");
    }
    macro.name = ((TokenType::Identifier)token.value()).sequence;
    // parameters
    token = streams.top().get();
    if(token == Token(TokenType::Punctuator(TokenType::Punctuator::Paren_L))){
        token = streams.top().get();
        while(token && std::holds_alternative<TokenType::Identifier>(token.value())){
            macro.params.emplace_back(((TokenType::Identifier)token.value()).sequence);
            token = streams.top().get();
            if(token){
                if(std::holds_alternative<TokenType::Punctuator>(token.value())){
                    if(token.value() == TokenType::Punctuator(TokenType::Punctuator::Comma)){
                        continue;
                    }else if(token.value() == TokenType::Punctuator(TokenType::Punctuator::Paren_R)){
                        token = streams.top().get();
                        break;
                    }else{
                        throw Exception::Error(pos, "unexpected token in macro parameter list");
                    }
                }
            }else{
                throw Exception::Error(pos, "unclosed parameter list in macro definition");
            }
        }
    }
    // replacements
    skip_whitespace(token);
    while(token && !std::holds_alternative<TokenType::NewLine>(token.value())){
        macro.replacement.emplace_back(token.value());
        token = streams.top().get();
    }

    // Check redifinition
    auto find_it = macros.find(macro);
    if((find_it != macros.end()) && (*find_it != macro)){
        Exception::Warning("existing macro redefined");
    }

    // Insert macro
    macros.emplace(macro);
}

PreProcessor::PPToken PreProcessor::get(){
    while(!streams.empty()){
        PPToken token = streams.top().get();
        do{
            skip_whitespace(token);
            if(token){
                // newline
                if(std::holds_alternative<TokenType::NewLine>(token.value())){
                    is_text = false;
                    token = streams.top().get();
                }else if(is_text){
                    return replace_macro(token, streams.top());
                }else{
                    if(std::holds_alternative<TokenType::Punctuator>(token.value()) &&
                        ((TokenType::Punctuator)token.value()).type == TokenType::Punctuator::Hash
                    ){
                        token = streams.top().get();
                        if(std::holds_alternative<TokenType::NewLine>(token.value())){
                            // Empty directive
                            is_text = false;
                            token = streams.top().get();
                        }else if(std::holds_alternative<TokenType::Identifier>(token.value())){
                            // Directive
                            TokenType::Identifier& identifier = std::get<TokenType::Identifier>(token.value());
                            if(identifier.sequence == "if"){
                                
                            }else if(identifier.sequence == "ifdef"){
                                
                            }else if(identifier.sequence == "ifndef"){
                                
                            }else if(identifier.sequence == "elif"){
                                
                            }else if(identifier.sequence == "else"){
                                
                            }else if(identifier.sequence == "endif"){
                                
                            }else if(identifier.sequence == "define"){
                                define_directive(token);
                                token = streams.top().get();
                            }else if(identifier.sequence == "undef"){
                                
                            }else if(identifier.sequence == "line"){
                                
                            }else if(identifier.sequence == "error"){
                                error_directive(token);
                            }else if(identifier.sequence == "pragma"){
                                
                            }
                        }else{
                            throw Exception::Error(token.value().pos, "Invalid preprocessor directive");
                        }
                    }else{
                        is_text = true;
                        return replace_macro(token, streams.top());
                    }
                }
            }
        }while(token);
        streams.pop();
    }
    return std::nullopt;
}

PreProcessor::PPToken& PreProcessor::replace_macro(PPToken& token, TokenStream& stream){
    if(token.hold<TokenType::Identifier>()){
        // Check if identifier is macro name
        for(Macro macro : macros){
            // Found macro
            if(token.value() == TokenType::Identifier(macro.name)){
                LineStream line;
                line.buffer.emplace_back(token.value());
            }
        }
        // Consume arguments
        std::optional<Token> next = stream.get();
        // for(std::list<Token>::iterator cur = line.begin(); cur != line.end();){
        //     bool modified = false;
        //     if(std::holds_alternative<TokenType::Identifier>(*cur)){
        //         // Get identifier
        //         TokenType::Identifier identifier = std::get<TokenType::Identifier>(*cur);
        //         // Iterate through macro
        //         for(Macro macro : macros){
        //             if(identifier.sequence == macro.name){
        //                 modified = true;
        //                 std::list<Token>::iterator next_cur = std::next(cur);
        //                 line.erase(cur);
        //                 if(macro.params.empty()){
        //                     /* Object like macro */
        //                     cur = line.insert(next_cur, macro.replacement.begin(), macro.replacement.end());
        //                     break;
        //                 }else{
        //                     /* Function like macro */ 
        //                     // Consume arguments
        //                     if(next_cur == )
        //                 }
        //             }
        //         }
        //     }
        //     if(!modified){
        //         cur = std::next(cur);
        //     }
        // }
        // token = line.buffer.front();
        // if(line.buffer.size() > 1){
        //     stream.buffer.insert(stream.buffer.end(), std::next(line.buffer.begin()), line.buffer.end());
        // }
    }
    return token;
}
