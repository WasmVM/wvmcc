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

typedef enum {
// Punctuators
	Punct_brackL,
	Punct_brackR,
	Punct_paranL,
	Punct_paranR,
	Punct_braceL,
	Punct_braceR,
	Punct_dot,
	Punct_arrow,
	Punct_inc,
	Punct_dec,
	Punct_amp,
	Punct_aster,
	Punct_plus,
	Punct_minus,
	Punct_tilde,
	Punct_exclm,
	Punct_slash,
	Punct_percent,
	Punct_shiftL,
	Punct_shiftR,
	Punct_lt,
	Punct_gt,
	Punct_le,
	Punct_ge,
	Punct_eq,
	Punct_neq,
	Punct_caret,
	Punct_vbar,
	Punct_and,
	Punct_or,
	Punct_ques,
	Punct_colon,
	Punct_semi,
	Punct_ellips,
	Punct_assign,
	Punct_ass_plus,
	Punct_ass_minus,
	Punct_ass_mult,
	Punct_ass_dev,
	Punct_ass_mod,
	Punct_ass_shl,
	Punct_ass_shr,
	Punct_ass_and,
	Punct_ass_neg,
	Punct_ass_or,
	Punct_comma,
	Punct_hash,
	Punct_hashhash,
	Punct_assiplus
} Punct;

typedef struct Node_ {
	NodeType type;
	union {
		unsigned long long int intVal;
		double floatVal;
		char *str;
		Punct punct;
		void *obj;
	} data;
	int unitSize;
	int byteLen;
	int isSigned;
} Node;

Node *getToken(FileInst *fileInst);

#endif