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

#include <Parser.hpp>

using namespace WasmVM;

Parser::Parser(Lexer& lexer) : lexer(lexer) {}

template<>
std::optional<AST::PrimaryExpr> Parser::parse<AST::PrimaryExpr>(){
    auto token = lexer.get();
    if(token){
        return std::visit<std::optional<AST::PrimaryExpr>>(overloaded {
            [](TokenType::Identifier& id){
                return id;
            },
            [](TokenType::StringLiteral& str){
                return str;
            },
            [](TokenType::IntegerConstant& val){
                return val;
            },
            [](TokenType::CharacterConstant& val){
                return val;
            },
            [](TokenType::FloatingConstant& val){
                return val;
            },
            [](auto&){
                // TODO:
                return std::nullopt;
            }
        }, token.value());
    }
    return std::nullopt;
}