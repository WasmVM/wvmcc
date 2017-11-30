#ifndef WASMCC_PP_LEX_DEF
#define WASMCC_PP_LEX_DEF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "token.h"

typedef struct PPLexer_ {
	PPToken *(*nextToken)(struct PPLexer_ *this);
	unsigned int codeSize;
	unsigned int curLine;
	unsigned int curPos;
	char *code;
	char *cur;
	char *filename;
} PPLexer;

PPLexer *initPPLexer(const char *fileName);
void freePPLexer(PPLexer **lexer);

#endif