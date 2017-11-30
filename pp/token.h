#ifndef WASMCC_PP_TOKEN_DEF
#define WASMCC_PP_TOKEN_DEF

typedef enum {
	PPHead,
	PPIdent,
	PPNum,
	PPChar,
	PPStr,
	PPPunct,
	PPOther
} PPTokenType;

typedef struct {
	PPTokenType type;
	char *str;
	unsigned int line;
	unsigned int pos;
} PPToken;

void freePPToken(PPToken **token);

#endif // !WASMCC_PP_TOKEN_DEF