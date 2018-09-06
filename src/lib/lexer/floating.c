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

static Token* suffix(char **inputPtr, double value){
    char* cursor = *inputPtr;
    if(*cursor == 'f' || *cursor == 'F'){
        ++cursor;
        *inputPtr = cursor;
        return new_FloatingToken(value, 4);
    }else if(*cursor == 'l' || *cursor == 'L'){
        ++cursor;
    }
    *inputPtr = cursor;
    return new_FloatingToken(value, 8);
}

static Token* decimal(char **inputPtr){
    char* cursor = *inputPtr;
    double value = 0;
    // Decimal sequence (integer part)
    _Bool hasIntegerPart = 0;
    while(*cursor >= '0' && *cursor <= '9'){
        value *= 10;
        value += *cursor - '0';
        ++cursor;
        hasIntegerPart = 1;
    }
    if(*cursor == '.'){
        ++cursor;
        // Decimal sequence (fraction part)
        _Bool hasFractionPart = 0;
        for(int i = -1; *cursor >= '0' && *cursor <= '9'; --i){
            value += pow(10, i) * (*cursor - '0');
            ++cursor;
            hasFractionPart = 1;
        }
        if(hasIntegerPart || hasFractionPart){
            // Exponent part
            if(*cursor == 'e' || *cursor == 'E'){
                ++cursor;
                // Sign
                _Bool isPositive = 1;
                if(*cursor == '-' || *cursor == '+'){
                    isPositive = (*cursor == '-') ? 0 : 1;
                    ++cursor;
                }
                // Digit sequence
                if(*cursor >= '0' && *cursor <= '9'){
                    int expoment = 0;
                    while(*cursor >= '0' && *cursor <= '9'){
                        expoment *= 10;
                        expoment += *cursor - '0';
                        ++cursor;
                    }
                    value *= pow(10, (isPositive) ? expoment : -expoment);
                }else{
                    return NULL;
                }
            }
            *inputPtr = cursor;
            return suffix(inputPtr, value);
        }
    }else{
        // Exponent part
        if(*cursor == 'e' || *cursor == 'E'){
            ++cursor;
            // Sign
            _Bool isPositive = 1;
            if(*cursor == '-' || *cursor == '+'){
                isPositive = (*cursor == '-') ? 0 : 1;
                ++cursor;
            }
            // Digit sequence
            if(*cursor >= '0' && *cursor <= '9'){
                int expoment = 0;
                while(*cursor >= '0' && *cursor <= '9'){
                    expoment *= 10;
                    expoment += *cursor - '0';
                    ++cursor;
                }
                value *= pow(10, (isPositive) ? expoment : -expoment);
            }else{
                return NULL;
            }
            *inputPtr = cursor;
            return suffix(inputPtr, value);
        }
    }
    return NULL;
}

static Token* hexadecimal(char **inputPtr){
    char* cursor = *inputPtr;
    double value = 0;
    if(cursor[0] != '0' || (cursor[1] != 'x' && cursor[1] != 'X')){
        return NULL;
    }
    cursor += 2;
    // Hexadecimal sequence (integer part)
    _Bool hasIntegerPart = 0;
    while((*cursor >= '0' && *cursor <= '9') ||
          (*cursor >= 'a' && *cursor <= 'f') ||
          (*cursor >= 'A' && *cursor <= 'F')
    ){
        value *= 16;
        if(*cursor >= '0' && *cursor <= '9'){
            value += *cursor - '0';
        }else if(*cursor >= 'a' && *cursor <= 'f'){
            value += *cursor - 'a' + 10;
        }else{
            value += *cursor - 'A' + 10;
        }
        ++cursor;
        hasIntegerPart = 1;
    }
    if(*cursor == '.'){
        ++cursor;
        // Hexadecimal sequence (integer )
        _Bool hasFractionPart = 0;
        for(int i = -1; (*cursor >= '0' && *cursor <= '9') ||
            (*cursor >= 'a' && *cursor <= 'f') ||
            (*cursor >= 'A' && *cursor <= 'F')
        ; --i){
            if(*cursor >= '0' && *cursor <= '9'){
                value += pow(16, i) * (*cursor - '0');
            }else if(*cursor >= 'a' && *cursor <= 'f'){
                value += pow(16, i) * (*cursor - 'a' + 10);
            }else{
                value += pow(16, i) * (*cursor - 'A' + 10);
            }
            ++cursor;
            hasFractionPart = 1;
        }
        if(hasIntegerPart || hasFractionPart){
            // Exponent part
            if(*cursor == 'p' || *cursor == 'P'){
                ++cursor;
                // Sign
                _Bool isPositive = 1;
                if(*cursor == '-' || *cursor == '+'){
                    isPositive = (*cursor == '-') ? 0 : 1;
                    ++cursor;
                }
                // Digit sequence
                if(*cursor >= '0' && *cursor <= '9'){
                    int expoment = 0;
                    while(*cursor >= '0' && *cursor <= '9'){
                        expoment *= 10;
                        expoment += *cursor - '0';
                        ++cursor;
                    }
                    value *= pow(2, (isPositive) ? expoment : -expoment);
                }else{
                    return NULL;
                }
            }
            *inputPtr = cursor;
            return suffix(inputPtr, value);
        }
    }else{
        // Exponent part
        if(*cursor == 'p' || *cursor == 'P'){
            ++cursor;
            // Sign
            _Bool isPositive = 1;
            if(*cursor == '-' || *cursor == '+'){
                isPositive = (*cursor == '-') ? 0 : 1;
                ++cursor;
            }
            // Digit sequence
            if(*cursor >= '0' && *cursor <= '9'){
                int expoment = 0;
                while(*cursor >= '0' && *cursor <= '9'){
                    expoment *= 10;
                    expoment += *cursor - '0';
                    ++cursor;
                }
                value *= pow(2, (isPositive) ? expoment : -expoment);
            }else{
                return NULL;
            }
            *inputPtr = cursor;
            return suffix(inputPtr, value);
        }
    }
    return NULL;
}

Token* lex_Floating(char **inputPtr){
    Token* token = NULL;
    if( (token = hexadecimal(inputPtr)) ||
        (token = decimal(inputPtr))
    ){
        return token;
    }
    return NULL; 
}
