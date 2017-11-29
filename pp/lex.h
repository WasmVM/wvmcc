#ifndef WASMCC_PP_LEX_DEF
#define WASMCC_PP_LEX_DEF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

typedef struct {
	PPToken *(*nextToken)();
	char *code;
	char *cur;
} PPLexer;

PPLexer *initPPLexer(const char *fileName);
void freePPLexer(PPLexer **lexer);

#endif