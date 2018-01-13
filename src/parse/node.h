#ifndef WVMCC_NODE_DEF
#define WVMCC_NODE_DEF

#include <ctype.h>
#include <uchar.h>
#include <math.h>

#include "../fileInst.h"
#include "../errorMsg.h"

typedef enum {
// Tokens
	Tok_Keyword,
	Tok_Ident,
	Tok_Int,
	Tok_Float,
	Tok_Char,
	Tok_String,
	Tok_Punct,
	Tok_EOF
} NodeType;

typedef struct Node_ {
	NodeType type;
	union {
		unsigned long long int intVal;
		double floatVal;
		char *str;
	} data;
	int unitSize;
	int byteLen;
	int isSigned;
} Node;

Node *getToken(FileInst *fileInst);

#endif