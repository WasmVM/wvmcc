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

PreProcessor::PreProcessor(std::filesystem::path path, std::vector<std::filesystem::path> includes) : include_paths(includes)
{
    streams.emplace(new SourceStream<std::ifstream>(path, path));
}
PreProcessor::PreProcessor(std::filesystem::path path, std::string source, std::vector<std::filesystem::path> includes) : include_paths(includes)
{
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
        return true;
    }else if(line.front().hold<TokenType::Punctuator>() && line.front().value() == TokenType::Punctuator(TokenType::Punctuator::Hash)){
        line.type = Line::Directive;
    }else{
        line.type = Line::Text;
    }
    return true;
}

PreProcessor::PPToken PreProcessor::get(){
    // Fill line
    if(line.empty()){
        // PP Directives
        while(readline() && line.type != Line::Text){
            Line::iterator cur = skip_whitespace(std::next(line.begin()), line.end());
            if(cur != line.end() && cur->hold<TokenType::Identifier>()){
                SourcePos pos = cur->value().pos;
                std::string direcitve_name = ((TokenType::Identifier)cur->value()).sequence;
                line.erase(line.begin(), std::next(cur));
                if(direcitve_name == "if"){
                    replace_macro(line, macros, true);
                    if_directive();
                }else if(direcitve_name == "ifdef"){
                    ifdef_ifndef_directive(true);
                }else if(direcitve_name == "ifndef"){
                    ifdef_ifndef_directive(false);
                }else if(direcitve_name == "elif"){
                    if_level -= 1;
                    if(if_level < 0){
                        throw Exception::Error(pos, "#elif without #if");
                    }
                    else_directive();
                }else if(direcitve_name == "else"){
                    if_level -= 1;
                    if(if_level < 0){
                        throw Exception::Error(pos, "#else without #if");
                    }
                    if(!line.empty()){
                        Exception::Warning("extra tokens at end of #else directive");
                    }
                    else_directive();
                }else if(direcitve_name == "endif"){
                    if_level -= 1;
                    if(if_level < 0){
                        throw Exception::Error(pos, "#endif without #if");
                    }
                    if(!line.empty()){
                        Exception::Warning("extra tokens at end of #endif directive");
                    }
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
                }else if(direcitve_name == "include"){
                    include_directive();
                }else{
                    throw Exception::Error(pos, "unknown preprocessor directive");
                }
            }
        }
        if(line.type != Line::Text){
            if(if_level != 0){
                throw Exception::Exception("unterminated conditional directive");
            }
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
            return val != 0;
        }
    }, expr.eval());
}

void PreProcessor::defined_operator(){
    Line::iterator cur = skip_whitespace(line.begin(), line.end());
    while(cur != line.end()){
        if(cur->hold<TokenType::Identifier>() && (((TokenType::Identifier)cur->value()).sequence == "defined")){
            Line::iterator start = cur;
            cur = skip_whitespace(std::next(cur), line.end());
            bool has_paren = false;
            if(cur != line.end() && cur->hold<TokenType::Punctuator>() && ((TokenType::Punctuator)cur->value() == TokenType::Punctuator::Paren_L)){
                has_paren = true;
                cur = skip_whitespace(std::next(cur), line.end());
            }
            if(cur != line.end() && cur->hold<TokenType::Identifier>()){
                std::string macro_name = ((TokenType::Identifier)cur->value()).sequence;
                line.emplace(start, Token(TokenType::PPNumber(macros.contains(macro_name) ? "1" : "0")));
                cur = skip_whitespace(std::next(cur), line.end());
            }else{
                throw Exception::Error(start->value().pos, "missing identifier in 'defined' operator");
            }
            if(has_paren){
                if(cur != line.end() && cur->hold<TokenType::Punctuator>() && ((TokenType::Punctuator)cur->value() == TokenType::Punctuator::Paren_R)){
                    cur = skip_whitespace(std::next(cur), line.end());
                }else{
                    throw Exception::Error(start->value().pos, "unclosed 'defined' operator");
                }
            }
            cur = line.erase(start, cur);
        }else{
            cur = std::next(cur);
        }
    }
}

void PreProcessor::jump_else(){
    size_t nest_level = 0;
    while(readline()){
        if(line.type == Line::Directive){
            Line::iterator cur = skip_whitespace(std::next(line.begin()), line.end());
            if(cur != line.end() && cur->hold<TokenType::Identifier>()){
                std::string direcitve_name = ((TokenType::Identifier)cur->value()).sequence;
                line.erase(line.begin(), std::next(cur));
                if(direcitve_name == "if"){
                    nest_level += 1;
                }else if((direcitve_name == "endif") || (direcitve_name == "else") || (direcitve_name == "elif")){
                    if(nest_level == 0){
                        if(direcitve_name == "elif"){
                            defined_operator();
                            // replace identifier with 0
                            for(PPToken& token : line){
                                if(token.hold<TokenType::Identifier>()){
                                    token.emplace(Token(TokenType::PPNumber("0")));
                                }
                            }
                            if(evaluate_condition()){
                                if_level += 1;
                                return;
                            }else{
                                continue;
                            }
                        }
                        if(!line.empty()){
                            Exception::Warning("extra tokens at end of #endif or #else directive");
                        }
                        if(direcitve_name == "else"){
                            if_level += 1;
                        }
                        return;
                    }
                    if(direcitve_name == "endif"){
                        nest_level -= 1;
                    }
                }
            }
        }
    }
}

void PreProcessor::if_directive(){
    // defined operator
    defined_operator();
    // replace identifier with 0
    for(PPToken& token : line){
        if(token.hold<TokenType::Identifier>()){
            token.emplace(Token(TokenType::PPNumber("0")));
        }
    }
    if(evaluate_condition()){
        if_level += 1;
    }else{
        jump_else();
    }
}

void PreProcessor::else_directive(){
    // Jump to end
    size_t nest_level = 0;
    while(readline()){
        if(line.type == Line::Directive){
            Line::iterator cur = skip_whitespace(std::next(line.begin()), line.end());
            if(cur != line.end() && cur->hold<TokenType::Identifier>()){
                std::string direcitve_name = ((TokenType::Identifier)cur->value()).sequence;
                line.erase(line.begin(), std::next(cur));
                if(direcitve_name == "if"){
                    nest_level += 1;
                }else if(direcitve_name == "endif"){
                    if(nest_level == 0){
                        if(!line.empty()){
                            Exception::Warning("extra tokens at end of #endif directive");
                        }
                        return;
                    }else{
                        nest_level -= 1;
                    }
                }
            }
        }
    }
}

void PreProcessor::ifdef_ifndef_directive(bool if_def){
    Line::iterator cur = skip_whitespace(line.begin(), line.end());
    if(cur != line.end() && cur->hold<TokenType::Identifier>()){
        std::string macro_name = ((TokenType::Identifier)cur->value()).sequence;
        if(macros.contains(macro_name) ^ if_def){
            jump_else();
        }else{
            if_level += 1;
        }
    }else{
        throw Exception::Exception("expected identifier in #ifdef or #ifndef");
    }
}

void PreProcessor::include_directive(){
    replace_macro(line, macros);
    Line::iterator cur = skip_whitespace(line.begin(), line.end());
    if(cur != line.end() && cur->hold<TokenType::HeaderName>()){
        SourcePos pos = cur->value().pos;
        TokenType::HeaderName header = cur->value();
        cur = skip_whitespace(std::next(cur), line.end());
        if(cur != line.end()){
            throw Exception::Exception("extra tokens in #include");
        }
        std::filesystem::path header_path(header.sequence.substr(1, header.sequence.size() - 2));
        std::filesystem::path cur_parent = pos.path.parent_path();
        if(header.sequence.front() == '"' && std::filesystem::exists(cur_parent / header_path)){
            header_path = cur_parent / header_path;
            streams.emplace(new SourceStream<std::ifstream>(header_path, header_path));
            return;
        }
        for(std::filesystem::path include_path : include_paths){
            if(std::filesystem::exists(include_path / header_path)){
                header_path = include_path / header_path;
                streams.emplace(new SourceStream<std::ifstream>(header_path, header_path));
                return;
            }
        }
        throw Exception::Error(pos, "'" + header_path.string() + "' file not found");
    }else{
        throw Exception::Exception("expected header name in #include");
    }
}