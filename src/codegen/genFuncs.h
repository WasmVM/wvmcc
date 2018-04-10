#ifndef WVMCC_CODEGEN_GENFUNCS_DEF
#define WVMCC_CODEGEN_GENFUNCS_DEF

#include "../list.h"
#include "../parse/token.h"
#include "../parse/structs.h"
#include <string.h>

int gen_primary_expression_identifier(List *codeList, Token *tok, List *identList);
void gen_primary_expression_constant(List *codeList, Token *tok);

#endif