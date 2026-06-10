/* tests/standard/libc/stdlib/atexit.c — LIBC-stdlib-atexit-01 (C17 7.22.4.2,
 * 7.22.4.4p3). atexit registers handlers that run at normal termination in
 * REVERSE order of registration. Verify=stdout (see atexit.expected). */
#include <stdio.h>
#include <stdlib.h>

static void first_registered(void)  { puts("first-registered"); }  /* runs last */
static void second_registered(void) { puts("second-registered"); } /* runs first */

int main(void) {
    if (atexit(first_registered) != 0) return 1;
    if (atexit(second_registered) != 0) return 2;
    puts("main");
    return 0;
}
