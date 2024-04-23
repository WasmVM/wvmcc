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

#include <AST/Dump.hpp>
#include <string>

using namespace WasmVM;

struct indent {
    static size_t level;
    indent& operator++() {
        level += 1;
        return *this;
    }
    indent& operator--() {
        if(level > 0){
            level -= 1;
        }
        return *this;
    }
};
size_t indent::level = 0;

std::ostream& operator<< (std::ostream& os, indent in) {
    return os << std::string(2 * in.level, ' ');
}

std::ostream& operator<< (std::ostream& os, AST::PrimaryExpr&& prim){
    os << indent() << "<PrimaryExpression>" << std::endl;
    ++indent();
    std::visit(overloaded {
        [&](TokenType::Identifier& id){
            os << indent() << id << std::endl;
        },
        [&](TokenType::StringLiteral& str){
            os << indent() << str << std::endl;
        },
        [&](TokenType::IntegerConstant& val){
            os << indent() << val << std::endl;
        },
        [&](TokenType::CharacterConstant& val){
            os << indent() << val << std::endl;
        },
        [&](TokenType::FloatingConstant& val){
            os << indent() << val << std::endl;
        },
        [&](TokenType::Punctuator& punct){
            if(punct.type == TokenType::Punctuator::Paren_L){
                // TODO: expression
            }
        },
        [&](TokenType::Keyword& keyword){
            if(keyword.value == "_Generic"){
                // TODO: generic-selectiom
            }
        },
        [&](auto&){}
    }, prim);
    --indent();
    os << indent() << "</PrimaryExpression>" << std::endl;
    return os;
}