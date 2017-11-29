#ifndef WASMCC_PP_TOKEN_DEF
#define WASMCC_PP_TOKEN_DEF

#include <string>

enum PPTokenType{
	PPHead,
	PPIdent,
	PPNum,
	PPChar,
	PPStr,
	PPPunct,
	PPOther,
	PPNewLine,
	PPCatNewLine
};

class PPToken{
public:
	PPTokenType type;
	std::string str;
	unsigned int line;
	unsigned int pos;
};
#endif // !WASMCC_PP_TOKEN_DEF