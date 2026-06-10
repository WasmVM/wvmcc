/* tests/standard/libc/stdlib/quick_exit.c — LIBC-stdlib-quick_exit-01
 * (C17 7.22.4.3, 7.22.4.7). at_quick_exit registers a handler that
 * quick_exit must invoke before terminating. main calls quick_exit(3); the
 * handler runs and _Exit(0)s, so status 0 iff the handler ran. Verify=exit. */
#include <stdlib.h>

static void handler(void) { _Exit(0); }

int main(void) {
    if (at_quick_exit(handler) != 0) return 1;
    quick_exit(3); /* handler must run and terminate with status 0 */
}
