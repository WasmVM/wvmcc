/* tests/standard/libc/errno/error_macros.c — LIBC-errno-EDOM-01.
 * Verify=static-assert. C17 7.5p2: EDOM, EILSEQ, and ERANGE expand to
 * integer constant expressions of type int, with distinct positive values,
 * suitable for use in #if preprocessing directives. */
#include <errno.h>

_Static_assert(EDOM > 0, "EDOM must be positive");
_Static_assert(EILSEQ > 0, "EILSEQ must be positive");
_Static_assert(ERANGE > 0, "ERANGE must be positive");
_Static_assert(EDOM != EILSEQ, "EDOM and EILSEQ must be distinct");
_Static_assert(EDOM != ERANGE, "EDOM and ERANGE must be distinct");
_Static_assert(EILSEQ != ERANGE, "EILSEQ and ERANGE must be distinct");

/* Suitable for use in #if preprocessing directives (7.5p2). */
#if !defined(EDOM) || !defined(EILSEQ) || !defined(ERANGE)
#error "EDOM, EILSEQ, ERANGE must be macros"
#endif
#if EDOM <= 0 || EILSEQ <= 0 || ERANGE <= 0
#error "error macros must be positive in #if"
#endif
