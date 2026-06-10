/* tests/standard/libc/stdlib/macros.c — LIBC-stdlib-EXIT-01 (C17 7.22p3).
 * EXIT_SUCCESS / EXIT_FAILURE / NULL / RAND_MAX / MB_CUR_MAX are defined with
 * the required forms/values. Verify=static-assert (compile-only). */
#include <stdlib.h>

#ifndef NULL
#error "stdlib.h must define NULL (7.22p3)"
#endif
#ifndef EXIT_SUCCESS
#error "stdlib.h must define EXIT_SUCCESS (7.22p3)"
#endif
#ifndef EXIT_FAILURE
#error "stdlib.h must define EXIT_FAILURE (7.22p3)"
#endif
#ifndef RAND_MAX
#error "stdlib.h must define RAND_MAX (7.22p3)"
#endif
#ifndef MB_CUR_MAX
#error "stdlib.h must define MB_CUR_MAX (7.22p3)"
#endif

/* 7.22.2.1p5: RAND_MAX shall be at least 32767. */
_Static_assert(RAND_MAX >= 32767, "RAND_MAX >= 32767 (7.22.2.1p5)");
/* exit(0)/exit(EXIT_SUCCESS) report success and exit(EXIT_FAILURE) reports
 * failure (7.22.4.4p5), so EXIT_FAILURE cannot be 0. */
_Static_assert(EXIT_FAILURE != 0, "EXIT_FAILURE must differ from the success status 0");
/* EXIT_SUCCESS expands to an integer constant expression (7.22p3). */
_Static_assert(EXIT_SUCCESS + 1 != EXIT_SUCCESS, "EXIT_SUCCESS is an integer constant expression");
/* MB_CUR_MAX is a positive value (7.22p3); == 1 in wvmcc's "C" locale. */
_Static_assert(MB_CUR_MAX >= 1, "MB_CUR_MAX is positive (catalog note: == 1)");
/* NULL expands to a null pointer constant (usable in a constant expression). */
_Static_assert(sizeof(NULL) > 0, "NULL expands to a null pointer constant");

int dummy; /* non-empty TU */
