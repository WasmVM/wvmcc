#ifndef WASMCC_ERRORMSG_DEF
#define WASMCC_ERRORMSG_DEF

#define WASMCC_ERR_NON_PP_DIRECTIVE \
  "[PP] %s:%u Error: normal text line shall not begin with # token"
#define WASMCC_ERR_EXPECT_HEADER_NAME \
  "[PP] %s:%u Error: expected \"FILENAME\" or <FILENAME>"
#define WASMCC_ERR_H_CHAR_HEADER_NOEND \
  "[PP] %s:%u Error: expected > in the end of header name"
#define WASMCC_ERR_Q_CHAR_HEADER_NOEND \
  "[PP] %s:%u Error: expected \" in the end of header name"
#define WASMCC_ERR_EXPECT_MACRO_NAME "[PP] %s:%u Error: expected MACRONAME"
#define WASMCC_ERR_INVALID_IDENTIFIER                     \
  "[PP] %s:%u Error: invalid identifier format. Format: " \
  "[_a-zA-Z][_a-zA-Z0-9]*"
#define WASMCC_ERR_INVALID_MACRO_PARAM \
  "[PP] %s:%u Error: invalid parameter in macro definition"
#define WASMCC_ERR_INVALID_MACRO_ARGS \
  "[PP] %s:%u Error: invalid argument in macro"
#define WASMCC_ERR_INVALID_CHARACTER \
  "[PP] %s:%u Error: invalid character constant"
#define WASMCC_ERR_IDENTIFIER_TOO_LONG \
  "[PP] %s:%u Error: identifier too long. Max length is 64"
#define WASMCC_ERR_EXPECT_MACRO_PARAM \
  "[PP] %s:%u Error: expected parameters for macro %s"
#define WASMCC_ERR_EXPECT_RPARAN "[PP] %s:%u Error: expected )"
#define WASMCC_ERR_EXPECT_LPARAN "[PP] %s:%u Error: expected enclosed ("
#define WASMCC_ERR_EXPECT_EQ "[PP] %s:%u Error: expected )"
#define WASMCC_ERR_PP_UNKNOWN_CHARACTER \
  "[PP] %s:%u Error: unknown character '%c' in preprocess directive"
#define WASMCC_ERR_MACRO_PARAM_TOO_MORE \
  "[PP] %s:%u Error: too more parameters for macro %s"
#define WASMCC_ERR_MACRO_PARAM_TOO_FEW \
  "[PP] %s:%u Error: too few parameters for macro %s"
#define WASMCC_ERR_EXPECT_VA "[PP] %s:%u Error: expected '...' operator"
#define WASMCC_ERR_VA_NOT_END \
  "[PP] %s:%u Error: '...' operator should be at end of argument list"
#define WASMCC_ERR_NO_DIGIT_AFTER_EXPO_SIGN \
  "[PP] %s:%u Error: there must be digit sequence after exponent sign"
#define WASMCC_ERR_EXPECT_DIGIT "[PP] %s:%u Error: expected digit sequence"
#define WASMCC_ERR_ERROR_DIRECTIVE "[PP] %s:%u Error: %s"
#define WASMCC_ERR_CONTAIN_INVALID_CHARACTER \
  "[PP] %s:%u Error: there's invalid character before line end"
#endif