// Copyright (C) 2025 Luis Hsu
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

#include "Visitor.hpp"
#include <Error.hpp>
#include <exception.hpp>
#include <regex>

using namespace WasmVM;

PreProcessor::Visitor::Visitor(std::filesystem::path path, PreProcessor& pp) : path(path), pp(pp)
{
    fin.open(path);
    input = std::make_unique<antlr4::ANTLRInputStream>(fin);
    lexer = std::make_unique<PPLexer>(input.get());
    tokens = std::make_unique<antlr4::CommonTokenStream>(lexer.get());
    parser = std::make_unique<PPParser>(tokens.get());
}

bool PreProcessor::Visitor::fetch(){
    return std::any_cast<bool>(visitGroup(parser->group()));
}

std::any PreProcessor::Visitor::visitGroup(PPParser::GroupContext *ctx){
    return visitChildren(ctx);
}

std::any PreProcessor::Visitor::visitIdentifier_list(PPParser::Identifier_listContext *ctx){
    return visitChildren(ctx);
}

std::any PreProcessor::Visitor::visitLine_end(PPParser::Line_endContext* ctx){
    antlr4::tree::TerminalNode* newline = ctx->NewLine();
    if(newline){
        pp.buffer.emplace_back(Token {
            .type = Token::NewLine,
            .text = "\n",
            .pos = SourcePos {
                .file = path,
                .line = newline->getSymbol()->getLine(),
                .col = newline->getSymbol()->getCharPositionInLine() + 1
            }
        });
    }
    if(ctx->EOF()){
        if(newline == nullptr){
            Exception::Warning("no new-line in the end of file");
        }
        return false;
    }else{
        return true;
    }
}

std::any PreProcessor::Visitor::visitText_line(PPParser::Text_lineContext *ctx){
    std::list<Token> token_list;
    for(auto pp_token : ctx->pp_token()){
        token_list.emplace_back(std::any_cast<Token>(visitPp_token(pp_token)));
    }
    replace_macro(token_list, pp.macros);
    pp.buffer.insert(pp.buffer.end(), token_list.begin(), token_list.end());
    return visitLine_end(ctx->line_end());
}

std::any PreProcessor::Visitor::visitControl_line(PPParser::Control_lineContext *ctx){
    return visitChildren(ctx);
}

static std::string convert_trigraph(std::string text){
    std::string result = "";
    for(auto it = text.begin(); it != text.end();){
        if(*it == '?' && (std::next(it) != text.end()) && (*std::next(it) == '?') && (std::next(std::next(it)) != text.end())){
            switch(*std::next(std::next(it))){
                case '!':
                    result += "|";
                    it = std::next(std::next(std::next(it)));
                    continue;
                case ')':
                    result += "]";
                    it = std::next(std::next(std::next(it)));
                    continue;
                case '(':
                    result += "[";
                    it = std::next(std::next(std::next(it)));
                    continue;
                case '>':
                    result += "}";
                    it = std::next(std::next(std::next(it)));
                    continue;
                case '<':
                    result += "{";
                    it = std::next(std::next(std::next(it)));
                    continue;
                case '\'':
                    result += "^";
                    it = std::next(std::next(std::next(it)));
                    continue;
                case '-':
                    result += "~";
                    it = std::next(std::next(std::next(it)));
                    continue;
                case '/':
                    result += "\\";
                    it = std::next(std::next(std::next(it)));
                    continue;
                default:
                    break;
            }
        }
        result += *it;
        it = std::next(it);
    }
    return result;
}

std::any PreProcessor::Visitor::visitPp_token(PPParser::Pp_tokenContext *ctx){
    antlr4::Token* token = ctx->getStart();
    SourcePos pos {.file = path, .line = token->getLine(), .col = token->getCharPositionInLine() + 1};
    switch (token->getType()){
        case PPLexer::HeaderName:
            return Token {.type = Token::HeaderName, .text = convert_trigraph(token->getText()), .pos = pos};
        case PPLexer::Identifier:
            return Token {.type = Token::Identifier, .text = token->getText(), .pos = pos};
        case PPLexer::PPNumber:
            return Token {.type = Token::PPNumber, .text = token->getText(), .pos = pos};
        case PPLexer::CharConst:
            return Token {.type = Token::CharConst, .text = convert_trigraph(token->getText()), .pos = pos};
        case PPLexer::StringLiteral:
            return Token {.type = Token::StringLiteral, .text = convert_trigraph(token->getText()), .pos = pos};
        case PPLexer::Punctuator:
            if(token->getText() == "<:"){
                return Token {.type = Token::Punctuator, .text = "[", .pos = pos};
            }else if(token->getText() == ":>"){
                return Token {.type = Token::Punctuator, .text = "]", .pos = pos};
            }else if(token->getText() == "<%"){
                return Token {.type = Token::Punctuator, .text = "{", .pos = pos};
            }else if(token->getText() == "%>"){
                return Token {.type = Token::Punctuator, .text = "}", .pos = pos};
            }
            return Token {.type = Token::Punctuator, .text = convert_trigraph(token->getText()), .pos = pos};
        case PPLexer::ParenL:
        case PPLexer::ParenR:
        case PPLexer::Ellipsis:
        case PPLexer::Comma:
            return Token {.type = Token::Punctuator, .text = token->getText(), .pos = pos};
        case PPLexer::Hash:
            return Token {.type = Token::Punctuator, .text = "#", .pos = pos};
        case PPLexer::HashHash:
            return Token {.type = Token::Punctuator, .text = "##", .pos = pos};
        case PPLexer::BlockComment:
        case PPLexer::LineComment:
        case PPLexer::WhiteSpace:
            return Token {.type = Token::WhiteSpace, .text = " ", .pos = pos};
        default:
            throw Exception::SyntaxError(pos, "unknown pre-processor token '" + token->getText() + "'");
    }
}

std::any PreProcessor::Visitor::visitDefine_obj(PPParser::Define_objContext *ctx){
    std::string name = ctx->Identifier()->getText();
    Macro macro {.name = name};
    for(auto pp_token : ctx->pp_token()){
        macro.replacement.emplace_back(std::any_cast<Token>(visitPp_token(pp_token)));
    }
    if(pp.macros.contains(name)){
        if(pp.macros[name] != macro){
            SourcePos pos {
                .file = path,
                .line = ctx->getStart()->getLine(),
                .col = ctx->getStart()->getCharPositionInLine() + 1
            };
            throw Exception::SyntaxError(pos, "duplicated macro '" + name + "'");
        }
    }else{
        pp.macros[name] = macro;
    }
    return visitLine_end(ctx->line_end());
}

std::any PreProcessor::Visitor::visitDefine_func(PPParser::Define_funcContext *ctx){
    std::string name = ctx->Identifier()->getText();
    Macro macro {.name = name, .params = std::vector<std::string>()};
    if(ctx->identifier_list() != nullptr){
        for(auto param : ctx->identifier_list()->Identifier()){
            macro.params->emplace_back(param->getText());
        }
    }
    if(ctx->Ellipsis() != nullptr){
        macro.params->emplace_back("...");
    }
    for(auto param : ctx->pp_token()){
        macro.replacement.emplace_back(std::any_cast<Token>(visitPp_token(param)));
    }
    if(pp.macros.contains(name)){
        if(pp.macros[name] != macro){
            SourcePos pos {
                .file = path,
                .line = ctx->getStart()->getLine(),
                .col = ctx->getStart()->getCharPositionInLine() + 1
            };
            throw Exception::SyntaxError(pos, "duplicated macro '" + name + "'");
        }
    }else{
        pp.macros[name] = macro;
    }
    return visitLine_end(ctx->line_end());
}

void PreProcessor::Visitor::replace_macro(std::list<Token>& tokens, const std::unordered_map<std::string, Macro>& macros){
    for(auto cur_it = tokens.begin(); cur_it != tokens.end();){
        Token cur = *cur_it;
        if(cur.type == Token::Identifier && macros.contains(cur.text)){
            // Found macro
            const Macro& macro = macros.at(cur.text);
            std::unordered_map<std::string, Macro> next_macros = macros;
            next_macros.erase(cur.text);
            auto start_it = cur_it;
            if(macro.params.has_value()){
                // function-type macro
                cur_it = std::next(cur_it);
                while(cur_it != tokens.end() && cur_it->type == Token::WhiteSpace){
                    cur_it = std::next(cur_it);
                }
                if(cur_it != tokens.end() && cur_it->type == Token::Punctuator && cur_it->text == "("){
                    cur_it = std::next(cur_it);
                    // get arguments
                    std::list<std::list<Token>> args;
                    if(macro.params->size() > 0){
                        args.emplace_back();
                        int level = 0;
                        while(cur_it != tokens.end()){
                            if(cur_it->type == Token::Punctuator){
                                if(cur_it->text == "("){
                                    level += 1;
                                }else if(cur_it->text == ")"){
                                    if(level > 0){
                                        level -= 1;
                                    }else{
                                        break;
                                    }
                                }else if(cur_it->text == ","){
                                    if(level == 0){
                                        args.emplace_back();
                                    }
                                }
                            }
                            args.back().emplace_back(*cur_it);
                            cur_it = std::next(cur_it);
                        }
                        if(cur_it == tokens.end() || cur_it->text != ")"){
                            throw Exception::SyntaxError(start_it->pos, "function-like macro not close");
                        }
                    }else{
                        if(cur_it == tokens.end()){
                            throw Exception::SyntaxError(start_it->pos, "function-like macro not close");
                        }else if(cur_it->text != ")"){
                            throw Exception::SyntaxError(start_it->pos, "too many arguments for macro '" + macro.name +"'");
                        }
                    }
                    if(args.size() < macro.params->size()){
                        throw Exception::SyntaxError(start_it->pos, "too few arguments for macro '" + macro.name + "'");
                    }
                    // replace each argument
                    for(std::list<Token>& arg : args){
                        replace_macro(arg, next_macros);
                    }
                    // generate argument map
                    std::unordered_map<std::string, std::list<Token>> arg_map;
                    for(size_t i = 0; !args.empty(); ++i){
                        if(i < macro.params->size()){
                            if(i != 0){
                                args.front().pop_front();
                            }
                            if(macro.params->at(i) == "..."){
                                arg_map["__VA_ARGS__"] = args.front();
                            }else{
                                arg_map[macro.params->at(i)] = args.front();
                            }
                        }else if(macro.params->back() == "..."){
                            arg_map["__VA_ARGS__"].insert(arg_map["__VA_ARGS__"].end(), args.front().begin(), args.front().end());
                        }else{
                            throw Exception::SyntaxError(start_it->pos, "too many arguments for macro '" + macro.name + "'");
                        }
                        args.pop_front();
                    }
                    // replacement
                    cur_it = tokens.erase(start_it, std::next(cur_it));
                    auto next_it = cur_it;
                    for(auto rep_it = macro.replacement.begin(); rep_it != macro.replacement.end();){
                        if(rep_it->type == Token::Punctuator && rep_it->text == "#"){

                        }else{
                            std::list<Token>::iterator ins_it;
                            if(rep_it->type == Token::Identifier && arg_map.contains(rep_it->text)){
                                ins_it = tokens.insert(cur_it, arg_map[rep_it->text].begin(), arg_map[rep_it->text].end());
                            }else{
                                ins_it = tokens.insert(cur_it, *rep_it);
                            }
                            if(next_it == cur_it){
                                next_it = ins_it;
                            }
                            rep_it = std::next(rep_it);
                        }
                    }
                    cur_it = next_it;
                }
            }else{
                // object-type macro
                std::list<Token> replacement(macro.replacement.begin(), macro.replacement.end());
                replace_macro(replacement, next_macros);
                cur_it = tokens.erase(start_it, std::next(cur_it));
                cur_it = tokens.insert(cur_it, replacement.begin(), replacement.end());
            }
            continue;
        }
        cur_it = std::next(cur_it);
    }
}