#ifndef WVMCC_NODE_DEF
#define WVMCC_NODE_DEF

#include <ctype.h>
#include <uchar.h>
#include <math.h>
#include <stdint.h>

#include "../fileInst.h"
#include "../errorMsg.h"

typedef enum {
// Tokens
	Tok_Keyword = 1,
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
	Punct_brackL = 1,
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
	Punct_ass_div,
	Punct_ass_mod,
	Punct_ass_shl,
	Punct_ass_shr,
	Punct_ass_and,
	Punct_ass_xor,
	Punct_ass_or,
	Punct_comma,
	Punct_assiplus
} Punct;

typedef enum {
	Keyw_auto = 1,
	Keyw_break,
	Keyw_case,
	Keyw_char,
	Keyw_const,
	Keyw_continue,
	Keyw_default,
	Keyw_do,
	Keyw_double,
	Keyw_else,
	Keyw_enum,
	Keyw_extern,
	Keyw_float,
	Keyw_for,
	Keyw_goto,
	Keyw_if,
	Keyw_inline,
	Keyw_int,
	Keyw_long,
	Keyw_register,
	Keyw_restrict,
	Keyw_return,
	Keyw_short,
	Keyw_signed,
	Keyw_sizeof,
	Keyw_static,
	Keyw_struct,
	Keyw_switch,
	Keyw_typedef,
	Keyw_union,
	Keyw_unsigned,
	Keyw_void,
	Keyw_volatile,
	Keyw_while,
	Keyw_Alignas,
	Keyw_Alignof,
	Keyw_Atomic,
	Keyw_Bool,
	Keyw_Complex,
	Keyw_Generic,
	Keyw_Imaginary,
	Keyw_Noreturn,
	Keyw_Static_assert,
	Keyw_Thread_local
} Keyword;

typedef struct Node_ {
	NodeType type;
	union {
		unsigned long long int intVal;
		double floatVal;
		char *str;
		Punct punct;
		Keyword keyword;
		void *obj;
	} data;
	int unitSize;
	int byteLen;
	int isSigned;
} Node;

Node *getToken(FileInst *fileInst);
// Return 1 if expected, 0 if not expected
// Value only valid with punctuator and keyword type
intptr_t expectToken(FileInst* fileInst, NodeType type, int value); 

#endif