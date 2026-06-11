/* tests/standard/libc/stdlib/exit.c — LIBC-stdlib-exit-01 (C17 7.22.4.4).
 * exit runs the atexit handlers, then terminates with the given status.
 * main calls exit(3); the registered handler must run and itself _Exit(0),
 * so the observed status is 0 iff exit invoked the handler. Verify=exit. */
#include <stdlib.h>

static void handler(void) { _Exit(0); }

int main(void) {
    if (atexit(handler) != 0) return 1;
    exit(3); /* handler must run and terminate with status 0 */
}
