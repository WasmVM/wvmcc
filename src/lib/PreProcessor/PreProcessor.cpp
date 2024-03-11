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

#include <string>
#include <fstream>
#include <sstream>
#include <list>
#include <Error.hpp>
#include <variant>

using namespace WasmVM;

PreProcessor::PreProcessor(std::filesystem::path path){
    streams.emplace(new SourceStream<std::ifstream>(path, path));
}
PreProcessor::PreProcessor(std::filesystem::path path, std::string source){
    streams.emplace(new SourceStream<std::istringstream>(path, source));
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

void PreProcessor::if_directive(){
    // TODO: defined operator
    // TODO: replace identifier
    evaluate_condition();
    // TODO: 
}

PreProcessor::PPToken PreProcessor::get(){
    // Fill line
    if(line.empty()){
        // PP Directives
        while(readline() && line.type == Line::Directive){
            Line::iterator cur = skip_whitespace(std::next(line.begin()), line.end());
            if(cur != line.end() && cur->hold<TokenType::Identifier>()){
                std::string direcitve_name = ((TokenType::Identifier)cur->value()).sequence;
                line.erase(line.begin(), std::next(cur));
                if(direcitve_name == "if"){
                    replace_macro(line, macros);
                    if_directive();
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
        // Replace macro
        replace_macro(line, macros);
        // Insert newline
        Token newline(TokenType::NewLine(), line.back()->pos);
        newline.pos.col() += 1;
        line.emplace_back(newline);
    }
    // Result
    PreProcessor::PPToken token = line.front();
    line.pop_front();
    return token;
}

bool PreProcessor::evaluate_condition(){
    Expression expr(line.begin(), line.end());
    return std::visit(overloaded {
        [](auto val){
            std::cout << val << std::endl; // TODO:
            return val != 0;
        }
    }, expr.eval());
}
