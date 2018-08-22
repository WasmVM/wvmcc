/**
 *    Copyright 2018 Luis Hsu
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <adt/Token.h>

static void free_Token(Token** token){
    if((*token)->type == Token_Identifier){
        free((void*)((*token)->value.identifier));
    }else if((*token)->type == Token_String){
        free((void*)((*token)->value.string));
    }
    free(*token);
    *token = NULL;
}

Token* new_UnknownToken(){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_Unknown;
    token->free = free_Token;
    return token;
}

Token* new_KeywordToken(const Keyword keyword){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_Keyword;
    token->value.keyword = keyword;
    token->free = free_Token;
    return token;
}

Token* new_IdentifierToken(const char *identifier){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_Identifier;
    token->value.identifier = identifier;
    token->free = free_Token;
    return token;
}
Token* new_IntegerToken(const unsigned long long int value){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_Integer;
    token->value.integer = value;
    token->free = free_Token;
    return token;
}
Token* new_FloatingToken(const double value){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_Floating;
    token->value.floating = value;
    token->free = free_Token;
    return token;
}
Token* new_CharacterToken(const char value){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_Character;
    token->value.character = value;
    token->free = free_Token;
    return token;
}
Token* new_StringToken(const char *string){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_String;
    token->value.string = string;
    token->free = free_Token;
    return token;
}
Token* new_PunctuatorToken(const Punctuator punctuator){
    Token* token = (Token*) malloc(sizeof(Token));
    token->type = Token_Punctuator;
    token->value.punctuator = punctuator;
    token->free = free_Token;
    return token;
}