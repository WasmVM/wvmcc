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

#include <regex>
#include <string>
#include <sstream>
#include <list>
#include <Error.hpp>
#include <cstdint>
#include <variant>

using namespace WasmVM;

static std::string trim(std::string str){
    str.erase(str.find_last_not_of(" \t\v\f") + 1);
    str.erase(0, str.find_first_not_of(" \t\v\f"));
    return str;
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

void PreProcessor::replace_macro(Line& line, std::unordered_map<std::string, Macro> macro_map, bool keep_defined){
    for(auto cur = line.begin(); cur != line.end();){
        if(cur->hold<TokenType::Identifier>()){
            TokenType::Identifier identifier = cur->value();
            Line::iterator start = cur;
            // Keep defined operator
            if(keep_defined && (identifier.sequence == "defined")){
                cur = skip_whitespace(std::next(cur), line.end());
                bool has_paren = false;
                if(cur != line.end() && cur->hold<TokenType::Punctuator>() && ((TokenType::Punctuator)cur->value() == TokenType::Punctuator::Paren_L)){
                    cur = skip_whitespace(std::next(cur), line.end());
                    has_paren = true;
                }
                if(cur == line.end() || !cur->hold<TokenType::Identifier>()){
                    throw Exception::Error(start->value().pos, "missing identifier in 'defined' operator");
                }
                if(has_paren){
                    cur = skip_whitespace(std::next(cur), line.end());
                    if(cur == line.end() || !cur->hold<TokenType::Punctuator>() || ((TokenType::Punctuator)cur->value() != TokenType::Punctuator::Paren_R)){
                        throw Exception::Error(start->value().pos, "unclosed 'defined' operator");
                    }
                }
                cur = std::next(cur);
                continue;
            }
            // Find macro
            if(cur != line.end() && (!cur->expanded.contains(identifier.sequence)) && macro_map.contains(identifier.sequence)){
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
                            SourcePos pos;
                            if(arg.empty()){
                                pos = replaced_cur->value().pos;
                            }else{
                                for(PPToken& tok : arg){
                                    if(!tok.hold<TokenType::WhiteSpace>() && !tok.hold<TokenType::NewLine>()){
                                        literal += tok.value().str() + " ";
                                    }
                                }
                                literal = std::regex_replace(trim(literal), std::regex("\\\\"), "\\");
                                literal = std::regex_replace(literal, std::regex("\""), "\\\"");
                                pos = arg.front()->pos;
                            }
                            auto new_cur = replaced_line.insert(replaced_cur, Token(TokenType::StringLiteral("\"" + literal + "\""), pos));
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
                                str_source << prev->value().str();
                            }
                            if(next->has_value()){
                                str_source << next->value().str();
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