/* tests/standard/libc/assert/ndebug.c — LIBC-assert-NDEBUG-01 (C17 7.2p1).
 * Verify=exit. With NDEBUG defined before including <assert.h>, assert
 * expands to ((void)0): the argument expression is NOT evaluated and no
 * action is taken. Returns 0 on pass. */
#define NDEBUG
#include <assert.h>

int main(void) {
    int n = 0;
    assert(n++);      /* must be a no-op: argument not evaluated */
    if (n != 0) return 1;
    assert(0);        /* must not abort */
    (void)(assert(0), 0); /* must still expand to a (void) expression */
    return 0;
}
