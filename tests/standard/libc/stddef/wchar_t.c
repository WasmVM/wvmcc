/* tests/standard/libc/stddef/wchar_t.c — LIBC-stddef-wchar_t-01 (C17 7.19p2).
 * Verify=static-assert. wchar_t is an integer type whose range of values
 * can represent distinct codes for all members of the largest extended
 * character set; each member of the basic character set has a code value
 * equal to its value as the lone character in an integer character
 * constant. Catalog status: partial (wvmcc placeholder `int`). */
#include <stddef.h>

/* Basic-character-set members have equal char/wchar_t code values (7.19p2). */
_Static_assert(L'a' == 'a', "L'a' == 'a'");
_Static_assert(L'0' == '0', "L'0' == '0'");

/* A wide character constant has type wchar_t (6.4.4.4p11). */
_Static_assert(sizeof(L'a') == sizeof(wchar_t),
               "wide character constant has type wchar_t");
