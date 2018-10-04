#include <Lexer.h>

Token* lex_String(char **inputPtr){
    char* cursor = *inputPtr;
    Vector* strVec = new_Vector(sizeof(uint32_t), NULL);
    unsigned int byteSize = 1;
    // Prefix
    if(*cursor == 'L'){
        ++cursor;
        byteSize = 2;
    }else if(*cursor == 'U'){
        ++cursor;
        byteSize = 4;
    }else if(*cursor == 'u'){
        if(cursor[1] == '8'){
            cursor += 2;
        }else{
            ++cursor;
            byteSize = 2;
        }
    }
    // Get sequence
    if(*cursor == '\"'){
        ++cursor;
        while(*cursor && *cursor != '\"'){
            uint32_t value = 0;
            if(*cursor == '\n'){
                return NULL;
            }
            // Escape
            if(*cursor == '\\'){
                ++cursor;
                if(*cursor == 'a'){
                    value = '\a';
                    ++cursor;
                }else if(*cursor == 'b'){
                    value = '\b';
                    ++cursor;
                }else if(*cursor == 'f'){
                    value = '\f';
                    ++cursor;
                }else if(*cursor == 'n'){
                    value = '\n';
                    ++cursor;
                }else if(*cursor == 'r'){
                    value = '\r';
                    ++cursor;
                }else if(*cursor == 't'){
                    value = '\t';
                    ++cursor;
                }else if(*cursor == 'v'){
                    value = '\v';
                    ++cursor;
                }else if(*cursor >= '0' && *cursor <= '7'){
                    // Octal
                    for(size_t i = 0; i < 3 && *cursor >= '0' && *cursor <= '7'; ++i, ++cursor){
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
            }else{
                value = *(cursor++);
            }
            strVec->push_back(strVec, &value);
        }
        *inputPtr = ++cursor;
        uint32_t* uString = (uint32_t*) calloc(strVec->length + 1, sizeof(uint32_t));
        if(strVec->data){
            memcpy(uString, strVec->data, strVec->length * sizeof(uint32_t));
        }
        free_Vector(&strVec);
        return new_StringToken(uString, byteSize);
    }
    return NULL;
}