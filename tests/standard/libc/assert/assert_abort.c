/* tests/standard/libc/assert/assert_abort.c — LIBC-assert-assert-02
 * (C17 7.2.1.1p2). Verify=exit — INVERTED: this program must terminate
 * with a NONZERO status. When the argument compares equal to 0, assert
 * writes diagnostic information on stderr and calls abort(), so control
 * never reaches the `return 0` below. Reaching it (exit status 0) means
 * the assertion did not fire — a conformance failure. The harness must
 * register this row as pass-iff-nonzero (catalog note: "test expects
 * nonzero exit"). */
#include <assert.h>

int main(void) {
    int zero = 0;
    assert(zero != 0); /* fails: must diagnose on stderr and abort() */
    return 0;          /* reached only on non-conformance */
}
