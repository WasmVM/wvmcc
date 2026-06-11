/* tests/standard/libc/errno/errno_set.c — LIBC-errno-set-01. Verify=exit.
 * C17 7.5p3: errno is set to a nonzero value by library functions on error
 * (7.22.1.4p8: strtol stores ERANGE on range error and returns LONG_MAX),
 * and no library function ever sets errno to zero — a successful call must
 * not clear a previously stored nonzero value.
 * Kept small for the WasmVM interpreter: one short overflowing literal. */
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
int main(void) {
    /* Range error: 20 nines > LONG_MAX (LP64) -> ERANGE, LONG_MAX. */
    errno = 0;
    long v = strtol("99999999999999999999", 0, 10);
    if (v != LONG_MAX) return 1;
    if (errno != ERANGE) return 2;

    /* Success does not clear errno (never set to zero by the library). */
    errno = ERANGE;
    v = strtol("5", 0, 10);
    if (v != 5) return 3;
    if (errno == 0) return 4;
    return 0;
}
