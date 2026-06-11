/* tests/standard/libc/errno/errno_lvalue.c — LIBC-errno-errno-01. Verify=exit.
 * C17 7.5p2: errno expands to a modifiable lvalue of type int. It must be
 * settable and readable, and assignment through it must round-trip. */
#include <errno.h>
int main(void) {
    errno = 0;
    if (errno != 0) return 1;
    errno = 42;
    if (errno != 42) return 2;
    errno = errno + 1;
    if (errno != 43) return 3;
    return 0;
}
