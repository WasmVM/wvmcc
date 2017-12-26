#ifndef WVMCC_NODE_DEF
#define WVMCC_NODE_DEF

#include <ctype.h>
#include <uchar.h>

#include "../fileInst.h"
#include "../errorMsg.h"

typedef enum {
// Tokens
	Tok_Keyword,
	Tok_Ident,
	Tok_Int,
	Tok_Float,
	Tok_Enum,
	Tok_Char,
	Tok_String,
	Tok_Punct
} NodeType;

typedef struct Node_ {
	NodeType type;
	union {
		long int intVal;
		double floatVal;
		char *str;
	} data;
	int unitSize;
	int byteLen;
} Node;

Node *getToken(FileInst *fileInst);

#endif