/* tests/standard/libc/time/macros.c — LIBC-time-macros-01 (C17 7.27.1).
 * Verify=static-assert. <time.h> defines NULL, CLOCKS_PER_SEC (an
 * expression with type clock_t, 7.27.1p2) and TIME_UTC (an integer
 * constant greater than 0, 7.27.1p3). */
#include <time.h>

#ifndef NULL
#error "NULL not defined by <time.h>"
#endif

#ifndef CLOCKS_PER_SEC
#error "CLOCKS_PER_SEC not defined by <time.h>"
#endif

#ifndef TIME_UTC
#error "TIME_UTC not defined by <time.h>"
#endif

/* TIME_UTC expands to an integer constant greater than 0 (7.27.1p3),
 * so it is usable in a constant expression. */
_Static_assert(TIME_UTC > 0, "TIME_UTC is an integer constant > 0");

/* CLOCKS_PER_SEC expands to an expression with type clock_t (7.27.1p2). */
_Static_assert(_Generic(CLOCKS_PER_SEC, clock_t: 1, default: 0),
               "CLOCKS_PER_SEC has type clock_t");
