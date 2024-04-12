// Copyright (C) 2024 Luis Hsu
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
// along with wvmcc. If not, see <https://www.gnu.org/licenses/>.

#include <Lexer.hpp>
#include <Error.hpp>
#include <string>

using namespace WasmVM;

Lexer::Lexer(PreProcessor &pp) : pp(pp){}

std::optional<Token> Lexer::get(){      
    std::optional<Token> token = next(); 
    if(token.has_value()){                                                            
        return std::visit<std::optional<Token>>(overloaded {
            [&token](TokenType::Identifier& id){
                if(TokenType::Keyword::is_keyword(id.sequence)){
                    return Token(TokenType::Keyword(id.sequence), token->pos);
                }else{
                    return token.value();
                }
            },
            [&token](TokenType::PPNumber& num){
                if(num.type == TokenType::PPNumber::Int){
                    if(num.sequence.find('u') != std::string::npos || num.sequence.find('U') != std::string::npos){
                        return Token(TokenType::IntegerConstant(num.get<uintmax_t>()), token->pos);
                    }else{
                        return Token(TokenType::IntegerConstant(num.get<intmax_t>()), token->pos);
                    }
                }else{
                    return Token(num.get<long double>(), token->pos);
                }
            },
            [&](TokenType::StringLiteral& str){
                std::optional<Token> next_tok;
                for(next_tok = next(); next_tok && std::holds_alternative<TokenType::StringLiteral>(next_tok.value()); next_tok = next()){
                    try{
                        str += std::get<TokenType::StringLiteral>(next_tok.value());
                    }catch(Exception::Exception e){
                        throw Exception::SyntaxError(token.value().pos, e.what());
                    }
                }
                if(next_tok){
                    buffer.emplace_front(next_tok.value());
                }
                return Token(str, token->pos);
            },
            [&token](TokenType::HeaderName&){
                throw Exception::SyntaxError(token.value().pos, "header name is invalid in parser");
                return std::nullopt;
            },
            [&token](auto&){
                return token;
            }
        }, token.value());
    }else{
        return std::nullopt;
    }
}

std::optional<Token> Lexer::next(){
    PreProcessor::PPToken token;
    if(buffer.empty()){
        token = pp.get();
        while(token && (token.hold<TokenType::NewLine>() || token.hold<TokenType::WhiteSpace>())){
            token = pp.get();
        }
    }else{
        token = buffer.front();
        buffer.pop_front();
    }
    return token;
}