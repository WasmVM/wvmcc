#ifndef WASMCC_PP_LEX_DEF
#define WASMCC_PP_LEX_DEF

#include <vector>
#include <fstream>
#include <string>

#include "token.hpp"

class PPLexer{
public:
	PPLexer(const char *fileName);

private:
	std::vector<char> code;
	char *cur = nullptr;
};

#endif