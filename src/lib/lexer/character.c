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

#include <Lexer.h>

Token* lex_Character(char **inputPtr){
    char* cursor = *inputPtr;
    uint32_t value = 0;
    unsigned int byteSize = 1;
    // Prefix
    if(*cursor == 'L' || *cursor == 'u'){
        ++cursor;
        byteSize = 2;
    }else if(*cursor == 'U'){
        ++cursor;
        byteSize = 4;
    }
    // Get sequence
    if(*cursor == '\''){
        while(*(++cursor)){
            if(*cursor == '\n'){
                return NULL;
            }
            // Escape
            if(*cursor == '\\'){
                ++cursor;
                if(*cursor == 'a'){
                    value = '\a';
                }else if(*cursor == 'b'){
                    value = '\b';
                }else if(*cursor == 'f'){
                    value = '\f';
                }else if(*cursor == 'n'){
                    value = '\n';
                }else if(*cursor == 'r'){
                    value = '\r';
                }else if(*cursor == 't'){
                    value = '\t';
                }else if(*cursor == 'v'){
                    value = '\v';
                }else if(*cursor >= '0' && *cursor <= '7'){
                    // Octal
                    for(size_t i = 0; i < 2 && *cursor >= '0' && *cursor <= '7'; ++i, ++cursor){
                        value *= 8;
                        value += *cursor - '0';
                    }
                }else if(*cursor == 'x'){
                    ++cursor;
                    // Hexadecimal
                    for(size_t i = 0; i < (byteSize * 2) && (
                        (*cursor >= '0' && *cursor <= '9') ||
                        (*cursor >= 'a' && *cursor <= 'f') ||
                        (*cursor >= 'A' && *cursor <= 'F')
                    ); ++i, ++cursor){
                        value *= 16;
                        if(*cursor >= '0' && *cursor <= '9'){
                            value += *cursor - '0';
                        }else if(*cursor >= 'a' && *cursor <= 'f'){
                            value += *cursor - 'a' + 10;
                        }else{
                            value += *cursor - 'A' + 10;
                        }
                    }
                }else if(*cursor == 'u' || *cursor == 'U'){
                    // Universal
                    size_t charSize = (*cursor == 'u') ? 2 : 4;
                    ++cursor;
                    for(size_t i = 0; i < (charSize * 2) && (
                        (*cursor >= '0' && *cursor <= '9') ||
                        (*cursor >= 'a' && *cursor <= 'f') ||
                        (*cursor >= 'A' && *cursor <= 'F')
                    ); ++i, ++cursor){
                        value *= 16;
                        if(*cursor >= '0' && *cursor <= '9'){
                            value += *cursor - '0';
                        }else if(*cursor >= 'a' && *cursor <= 'f'){
                            value += *cursor - 'a' + 10;
                        }else{
                            value += *cursor - 'A' + 10;
                        }
                    }
                }else{
                    value = *(cursor++);
                }
            }else if(value == 0 && *cursor != '\''){
                value = *cursor;
            }
            if(*cursor == '\''){
                *inputPtr = ++cursor;
                return new_CharacterToken(value, byteSize);
            }
        }
    }
    return NULL;
}