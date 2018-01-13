#ifndef WVMCC_ERRORMSG_DEF
#define WVMCC_ERRORMSG_DEF

#define WVMCC_ERR_NON_PP_DIRECTIVE \
  "[PP] %s:%u Error: normal text line shall not begin with # token"
#define WVMCC_ERR_EXPECT_HEADER_NAME \
  "[PP] %s:%u Error: expected \"FILENAME\" or <FILENAME>"
#define WVMCC_ERR_H_CHAR_HEADER_NOEND \
  "[PP] %s:%u Error: expected > in the end of header name"
#define WVMCC_ERR_Q_CHAR_HEADER_NOEND \
  "[PP] %s:%u Error: expected \" in the end of header name"
#define WVMCC_ERR_EXPECT_MACRO_NAME "[PP] %s:%u Error: expected MACRONAME"
#define WVMCC_ERR_INVALID_IDENTIFIER                     \
  "[PP] %s:%u Error: invalid identifier format. Format: " \
  "[_a-zA-Z][_a-zA-Z0-9]*"
#define WVMCC_ERR_INVALID_MACRO_PARAM \
  "[PP] %s:%u Error: invalid parameter in macro definition"
#define WVMCC_ERR_INVALID_MACRO_ARGS \
  "[PP] %s:%u Error: invalid argument in macro"
#define WVMCC_ERR_INVALID_CHARACTER \
  "[PP] %s:%u Error: invalid character constant"
#define WVMCC_ERR_IDENTIFIER_TOO_LONG \
  "[PP] %s:%u Error: identifier too long. Max length is 64"
#define WVMCC_ERR_EXPECT_MACRO_PARAM \
  "[PP] %s:%u Error: expected parameters for macro %s"
#define WVMCC_ERR_EXPECT_RPARAN "[PP] %s:%u Error: expected )"
#define WVMCC_ERR_EXPECT_LPARAN "[PP] %s:%u Error: expected enclosed ("
#define WVMCC_ERR_EXPECT_EQ "[PP] %s:%u Error: expected )"
#define WVMCC_ERR_PP_UNKNOWN_CHARACTER \
  "[PP] %s:%u Error: unknown character '%c' in preprocess directive"
#define WVMCC_ERR_MACRO_PARAM_TOO_MORE \
  "[PP] %s:%u Error: too more parameters for macro %s"
#define WVMCC_ERR_MACRO_PARAM_TOO_FEW \
  "[PP] %s:%u Error: too few parameters for macro %s"
#define WVMCC_ERR_EXPECT_VA "[PP] %s:%u Error: expected '...' operator"
#define WVMCC_ERR_VA_NOT_END \
  "[PP] %s:%u Error: '...' operator should be at end of argument list"
#define WVMCC_ERR_NO_DIGIT_AFTER_EXPO_SIGN \
  "[PP] %s:%u Error: there must be digit sequence after exponent sign"
#define WVMCC_ERR_EXPECT_DIGIT "[PP] %s:%u Error: expected digit sequence"
#define WVMCC_ERR_ERROR_DIRECTIVE "[PP] %s:%u Error: %s"
#define WVMCC_ERR_CONTAIN_INVALID_CHARACTER \
  "[PP] %s:%u Error: there's invalid character before line end"
#define WVMCC_ERR_EXPECT_MORE_IN_IF \
  "[PP] %s:%u Error: expected more constant in #if or #elif directive"
#define WVMCC_ERR_EXPECT_STRING_LITERAL \
  "[CC] %s:%u Error: expected '\"' in string literal"
#define WVMCC_ERR_EXPECT_CHARACTER_CONSTANT \
  "[CC] %s:%u Error: expected ' in character constant"
#define WVMCC_ERR_UNKNOWN_TOKEN \
  "[CC] %s:%u Error: unknown token"
#define WVMCC_ERR_PP_INVALID_ESCAPE_CHARACTER \
  "[CC] %s:%u Error: invalid escape character"
#define WVMCC_ERR_EXPECT_DECIMAL_POINT \
  "[CC] %s:%u Error: expected '.' in floating constant"
#endif