/* tests/standard/libc/stdlib/_exit.c — LIBC-stdlib-_Exit-01 (C17 7.22.4.5).
 * _Exit terminates immediately with the given status WITHOUT running atexit
 * handlers. If _Exit wrongly drained the atexit list, the handler would
 * change the status to 4; if it returned, main would return 2. Verify=exit. */
#include <stdlib.h>

static void must_not_run(void) { _Exit(4); }

int main(void) {
    if (atexit(must_not_run) != 0) return 1;
    _Exit(0);
    return 2; /* unreachable in a conforming implementation */
}
