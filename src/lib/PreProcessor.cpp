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
            if(wh == '('){
                return Token(TokenType::Punctuator(TokenType::Punctuator::Paren_L), source.position());
            }else{
                source.unget();
                return get();
            }
        }break;
        case '[' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_L), source.position());
        break;
        case ']' :
            return Token(TokenType::Punctuator(TokenType::Punctuator::Bracket_R), source.position());
        break;
        case '(' :
            return Token(TokenType::Lparen(), source.position());
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
    return std::nullopt;
}