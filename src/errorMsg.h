#ifndef WASMCC_ERRORMSG_DEF
#define WASMCC_ERRORMSG_DEF

#define WASMCC_ERR_NON_PP_DIRECTIVE "[PP] %s:%u Error: normal text line shall not begin with # token"
#define WASMCC_ERR_EXPECT_HEADER_NAME "[PP] %s:%u Error: expected \"FILENAME\" or <FILENAME>"
#define WASMCC_ERR_H_CHAR_HEADER_NOEND "[PP] %s:%u Error: expected > in the end of header name"
#define WASMCC_ERR_Q_CHAR_HEADER_NOEND "[PP] %s:%u Error: expected \" in the end of header name"
#define WASMCC_ERR_EXPECT_MACRO_NAME "[PP] %s:%u Error: expected MACRONAME"
#define WASMCC_ERR_INVALID_IDENTIFIER "[PP] %s:%u Error: invalid identifier format. Format: [_a-zA-Z][_a-zA-Z0-9]*"
#define WASMCC_ERR_IDENTIFIER_TOO_LONG "[PP] %s:%u Error: identifier too long, must be less than 64 in length"
#define WASMCC_ERR_INVALID_MACRO_PARAM "[PP] %s:%u Error: invalid parameter in macro definition"
#define WASMCC_ERR_EXPECT_VA "[PP] %s:%u Error: expected '...' operator"
#define WASMCC_ERR_VA_NOT_END "[PP] %s:%u Error: '...' operator should be at end of argument list"
#endif