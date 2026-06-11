/* tests/standard/libc/assert/assert_nonzero.c — LIBC-assert-assert-01
 * (C17 7.2.1.1p2). Verify=exit. When the argument compares unequal to 0,
 * the assert macro takes no action; it is an expression of type void
 * (usable as the left operand of a comma operator). The argument IS
 * evaluated in this (non-NDEBUG) form. Returns 0 on pass. */
#include <assert.h>

int main(void) {
    int n = 0;
    assert(++n == 1);          /* true: no action; side effect happens */
    if (n != 1) return 1;
    assert(42);                /* nonzero scalar: no action */
    int r = (assert(n == 1), 7); /* void expression in a comma expression */
    if (r != 7) return 2;
    return 0;
}
