#ifndef WVMCC_LEXER_DEF
#define WVMCC_LEXER_DEF

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <adt/Token.h>
#include <TokenBuffer.h>
#include <ByteBuffer.h>
#include <adt/Pass.h>

typedef Pass Lexer;
Pass* new_Lexer(Buffer** input, size_t input_count, Buffer** output, size_t output_count);
Token* lex_Keyword(char **inputPtr);
Token* lex_Identifier(char **inputPtr);
Token* lex_Integer(char **inputPtr);
Token* lex_Floating(char **inputPtr);
Token* lex_Character(char **inputPtr);
Token* lex_String(char **inputPtr);
Token* lex_Punctuator(char **inputPtr);

#endif