/* LANG-4-03 — 4p6: a conforming freestanding implementation shall accept any
 * strictly conforming program in which the use of the library features is
 * confined to the contents of <float.h>, <iso646.h>, <limits.h>,
 * <stdalign.h>, <stdarg.h>, <stdbool.h>, <stddef.h>, <stdint.h> and
 * <stdnoreturn.h>. Each header must be includable, and each must provide
 * its required definitions. */

#include <float.h>
#include <iso646.h>
#include <limits.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

/* <float.h> — 5.2.4.2.2: radix of floating-point representations. */
_Static_assert(FLT_RADIX >= 2, "float.h defines FLT_RADIX >= 2");

/* <iso646.h> — 7.9: alternative spellings expand to the operators. */
_Static_assert((1 and 1) == 1, "iso646.h defines 'and'");
_Static_assert((0 or 1) == 1, "iso646.h defines 'or'");
_Static_assert((not 0) == 1, "iso646.h defines 'not'");

/* <limits.h> — 5.2.4.2.1: CHAR_BIT is at least 8. */
_Static_assert(CHAR_BIT >= 8, "limits.h defines CHAR_BIT >= 8");
_Static_assert(INT_MAX >= 32767, "limits.h defines INT_MAX");

/* <stdalign.h> — 7.15: alignas/alignof macros. */
_Static_assert(alignof(char) == 1, "stdalign.h defines alignof");
#ifndef __alignas_is_defined
#error "stdalign.h must define __alignas_is_defined"
#endif
#ifndef __alignof_is_defined
#error "stdalign.h must define __alignof_is_defined"
#endif

/* <stdarg.h> — 7.16: va_list is a complete object type. */
_Static_assert(sizeof(va_list) > 0, "stdarg.h declares va_list");

/* <stdbool.h> — 7.18: bool/true/false macros. */
_Static_assert(true == 1, "stdbool.h defines true as 1");
_Static_assert(false == 0, "stdbool.h defines false as 0");
_Static_assert(sizeof(bool) == sizeof(_Bool), "stdbool.h defines bool as _Bool");
#ifndef __bool_true_false_are_defined
#error "stdbool.h must define __bool_true_false_are_defined"
#endif

/* <stddef.h> — 7.19: size_t, ptrdiff_t, NULL, offsetof. */
_Static_assert(sizeof(size_t) > 0, "stddef.h declares size_t");
_Static_assert(sizeof(ptrdiff_t) > 0, "stddef.h declares ptrdiff_t");
_Static_assert((int)(NULL == 0), "stddef.h defines NULL");
struct lang_4_03_s { char a; int b; };
_Static_assert(offsetof(struct lang_4_03_s, a) == 0, "stddef.h defines offsetof");

/* <stdint.h> — 7.20: exact-width and minimum-width integer types/limits. */
_Static_assert(sizeof(int32_t) == 4, "stdint.h declares int32_t");
_Static_assert(INT32_MAX == 2147483647, "stdint.h defines INT32_MAX");
_Static_assert(sizeof(intmax_t) >= 8, "stdint.h declares intmax_t");

/* <stdnoreturn.h> — 7.23: noreturn expands to _Noreturn. */
noreturn void lang_4_03_never_returns(void);
