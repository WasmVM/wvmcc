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

#include <AST/Expr.hpp>
#include <Error.hpp>

using namespace WasmVM;
using namespace AST;

std::optional<PrimaryExpr> PrimaryExpr::parse(Lexer& lexer){
    std::optional<Token> token = lexer.get();
    if(token){
        return std::visit<std::optional<PrimaryExpr>>(overloaded {
            [](TokenType::Identifier& value){
                return PrimaryExpr(value);
            },
            [](TokenType::StringLiteral& value){
                return PrimaryExpr(value);
            },
            [](TokenType::IntegerConstant& value){
                return PrimaryExpr(value);
            },
            [](TokenType::FloatingConstant& value){
                return PrimaryExpr(value);
            },
            [](TokenType::CharacterConstant& value){
                return PrimaryExpr(value);
            },
            [](auto&){
                return std::nullopt;
            }
        }, token.value());
    }
    return std::nullopt;
}