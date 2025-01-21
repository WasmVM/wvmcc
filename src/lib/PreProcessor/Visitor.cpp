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

using namespace WasmVM;

PreProcessor::Visitor::Visitor(std::filesystem::path path, PreProcessor& pp) : path(path), pp(pp)
{
    fin.open(path);
    input = std::make_unique<antlr4::ANTLRInputStream>(fin);
    lexer = std::make_unique<PPLexer>(input.get());
    tokens = std::make_unique<antlr4::CommonTokenStream>(lexer.get());
    parser = std::make_unique<PPParser>(tokens.get());
    parser->removeErrorListeners();
    parser->addErrorListener(&parseErrorListener);
}

bool PreProcessor::Visitor::fetch(){
    auto* group = parser->group();
    visitGroup(group);
    return !parser->isMatchedEOF();
}

std::any PreProcessor::Visitor::visitGroup(PPParser::GroupContext *ctx){
    return visitChildren(ctx);
}

std::any PreProcessor::Visitor::visitText_line(PPParser::Text_lineContext *ctx){
    for(auto pp_token : ctx->pp_token()){
        pp.buffer.emplace_back(std::any_cast<Token>(visitPp_token(pp_token)));
    }
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
        return ctx->pp_token().size() + 1;
    }else{
        return ctx->pp_token().size();
    }
}

std::any PreProcessor::Visitor::visitPp_token(PPParser::Pp_tokenContext *ctx){
    antlr4::Token* token = ctx->getStart();
    SourcePos pos {.file = path, .line = token->getLine(), .col = token->getCharPositionInLine() + 1};
    switch (token->getType()){
        case PPLexer::HeaderName:
            return Token {.type = Token::HeaderName, .text = token->getText(), .pos = pos};
        case PPLexer::Identifier:
            return Token {.type = Token::Identifier, .text = token->getText(), .pos = pos};
        case PPLexer::PPNumber:
            return Token {.type = Token::PPNumber, .text = token->getText(), .pos = pos};
        case PPLexer::CharConst:
            return Token {.type = Token::CharConst, .text = token->getText(), .pos = pos};
        case PPLexer::StringLiteral:
            return Token {.type = Token::StringLiteral, .text = token->getText(), .pos = pos};
        case PPLexer::Punctuator:
            return Token {.type = Token::Punctuator, .text = token->getText(), .pos = pos};
        case PPLexer::BlockComment:
        case PPLexer::LineComment:
        case PPLexer::WhiteSpace:
            return Token {.type = Token::WhiteSpace, .text = " ", .pos = pos};
        default:
            throw Exception::SyntaxError(pos, std::string("unknown pre-processor token '") + token->getText() + "'");
    }
}