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
// along with wvmcc. If not, see <http://www.gnu.org/licenses/>.

#include <PreProcessor.hpp>

#include <cerrno>
#include <limits>
#include <functional>
#include <Error.hpp>

using namespace WasmVM;

PreProcessor::Expression::Expression(Line::iterator cur, Line::iterator end) : cur(cur), end(end){}

template<typename T>
PreProcessor::Expression::Result& PreProcessor::Expression::cast_result(PreProcessor::Expression::Result& res){
    T val = std::visit<T>(overloaded {
        [](auto val){
            return (T)val;
        }
    }, res);
    res = val;
    return res;
}

void PreProcessor::Expression::implicit_cast(PreProcessor::Expression::Result& op1, PreProcessor::Expression::Result& op2){
    if(op1.index() != op2.index()){
        if(std::holds_alternative<double>(op1) || std::holds_alternative<double>(op2)){
            op1 = cast_result<double>(op1);
            op2 = cast_result<double>(op2);
        }else if(std::holds_alternative<uintmax_t>(op1) || std::holds_alternative<uintmax_t>(op2)){
            op1 = cast_result<uintmax_t>(op1);
            op2 = cast_result<uintmax_t>(op2);
        }
    }
}

template<template<typename T> class Op, typename T>
    requires std::is_same_v<Op<T>, std::modulus<T>>
PreProcessor::Expression::Result PreProcessor::Expression::binary_op(PreProcessor::Expression::Result& op1, PreProcessor::Expression::Result& op2){
    return std::visit<Result>(overloaded {
        [&](uintmax_t op1){
            return Op()(op1, std::get<uintmax_t>(op2));
        },
        [&](intmax_t op1){
            return Op()(op1, std::get<intmax_t>(op2));
        },
        [](double){
            // Should be integral
            return std::numeric_limits<double>::quiet_NaN();
        }
    }, op1);
}

template<template<typename T> class Op, typename T>
PreProcessor::Expression::Result PreProcessor::Expression::binary_op(PreProcessor::Expression::Result& op1, PreProcessor::Expression::Result& op2){
    return std::visit<Result>(overloaded {
        [&](double op1){
            Op<double> op;
            return op(op1, std::get<double>(op2));
        },
        [&](uintmax_t op1){
            Op<uintmax_t> op;
            return op(op1, std::get<uintmax_t>(op2));
        },
        [&](intmax_t op1){
            Op<intmax_t> op;
            return op(op1, std::get<intmax_t>(op2));
        }
    }, op1);
}

PreProcessor::Expression::Result PreProcessor::Expression::eval(){
    return additive(); // TODO:
}

PreProcessor::Expression::Result PreProcessor::Expression::primary(){
    cur = skip_whitespace(cur, end);
    if(cur == end){
        throw Exception::Exception("empty primary expression of preprocessor");
    }
    if(cur->hold<TokenType::Punctuator>() && (((TokenType::Punctuator)cur->value()).type == TokenType::Punctuator::Paren_L)){
        cur = std::next(cur);
        PreProcessor::Expression::Result result = eval();
        if(cur == end || !cur->hold<TokenType::Punctuator>() || !(((TokenType::Punctuator)cur->value()).type == TokenType::Punctuator::Paren_R)){
            throw Exception::Error(std::prev(cur)->value().pos, "expect ')' in proprocessor primary expression");
        }
        cur = std::next(cur);
        return result;
    }else if(cur->hold<TokenType::CharacterConstant>()){
        TokenType::CharacterConstant ch = cur->value();
        cur = std::next(cur);
        return std::visit(overloaded {
            [](auto val){
                return (intmax_t) val;
            }
        }, ch.value);
    }else if(cur->hold<TokenType::PPNumber>()){
        TokenType::PPNumber num = cur->value();
        cur = std::next(cur);
        if(num.type == TokenType::PPNumber::Float){
            return num.get<double>();
        }else{
            intmax_t res = num.get<intmax_t>();
            if(errno == ERANGE){
                return num.get<uintmax_t>();
            }else{
                return res;
            }
        }
    }
    throw Exception::Error(cur->value().pos, "invalid token in expression of preprocessor");
}

PreProcessor::Expression::Result PreProcessor::Expression::unary(){
    cur = skip_whitespace(cur, end);
    if(cur == end){
        throw Exception::Exception("empty unary expression of preprocessor");
    }
    if(cur->hold<TokenType::Punctuator>()){
        TokenType::Punctuator punct = cur->value();
        switch(punct.type){
            case TokenType::Punctuator::Plus :
                cur = std::next(cur);
                return multiplicative();
            case TokenType::Punctuator::Minus :
                cur = std::next(cur);
                return std::visit<Result>(overloaded {
                    [](auto val){
                        return -val;
                    }
                }, multiplicative());
            case TokenType::Punctuator::Tlide :
                cur = std::next(cur);
                return std::visit<Result>(overloaded {
                    [&](double val){
                        throw Exception::Error(std::prev(cur)->value().pos, "operand of bitwise '~' should be integral");
                        return std::numeric_limits<double>::quiet_NaN();
                    },
                    [](auto val){
                        return ~val;
                    }
                }, multiplicative());
            case TokenType::Punctuator::Exclam :
                cur = std::next(cur);
                return std::visit<intmax_t>(overloaded {
                    [](double val){
                        return (val == 0.0) ? 1 : 0;
                    },
                    [](auto val){
                        return (val == 0) ? 1 : 0;
                    }
                }, multiplicative());
            break;
            default:
            break;
        }
    }
    return primary();
}

PreProcessor::Expression::Result PreProcessor::Expression::multiplicative(){
    PreProcessor::Expression::Result res = unary();
    Line::iterator head = skip_whitespace(cur, end);
    cur = head;
    if(cur != end && cur->hold<TokenType::Punctuator>()){
        TokenType::Punctuator punct = cur->value();
        if(punct.type == TokenType::Punctuator::Aster || punct.type == TokenType::Punctuator::Slash || punct.type == TokenType::Punctuator::Perce){
            cur = std::next(cur);
            PreProcessor::Expression::Result operand = multiplicative();
            implicit_cast(res, operand);
            if(punct.type == TokenType::Punctuator::Aster){
                return binary_op<std::multiplies>(res, operand);
            }else if(punct.type == TokenType::Punctuator::Slash){
                std::visit(overloaded {
                    [&](auto val){
                        if(val == 0){
                            throw Exception::Error(head->value().pos, "divided by zero");
                        }
                    }
                }, operand);
                return binary_op<std::divides>(res, operand);
            }else{
                if(std::holds_alternative<double>(res) || std::holds_alternative<double>(operand)){
                    throw Exception::Error(head->value().pos, "operands of '%' should be integral");
                }
                std::visit(overloaded {
                    [&](auto val){
                        if(val == 0){
                            throw Exception::Error(head->value().pos, "divided by zero");
                        }
                    }
                }, operand);
                return binary_op<std::modulus>(res, operand);
            }
        }
    }
    return res;
}

PreProcessor::Expression::Result PreProcessor::Expression::additive(){
    PreProcessor::Expression::Result res = multiplicative();
    Line::iterator head = skip_whitespace(cur, end);
    cur = head;
    if(cur != end && cur->hold<TokenType::Punctuator>()){
        TokenType::Punctuator punct = cur->value();
        if(punct.type == TokenType::Punctuator::Plus || punct.type == TokenType::Punctuator::Minus){
            cur = std::next(cur);
            PreProcessor::Expression::Result operand = additive();
            implicit_cast(res, operand);
            if(punct.type == TokenType::Punctuator::Plus){
                return binary_op<std::plus>(res, operand);
            }else{
                return binary_op<std::minus>(res, operand);
            }
        }
    }
    return res;
}