/* tests/standard/libc/assert/assert_diag.c — LIBC-assert-assert-03
 * (C17 7.2.1.1p2). Verify=stdout (B-impl). The failure diagnostic must
 * include the text of the argument, __FILE__, __LINE__, and __func__
 * (the exact message format is implementation-defined; the fixture
 * encodes wvmcc's documented format, extended with the required line
 * number). #line pins __FILE__/__LINE__ so the fixture is independent
 * of the build tree. NOTE: the standard sends the diagnostic to STDERR;
 * the harness must capture it (run with 2>&1) before diffing against
 * assert_diag.expected. */
#include <assert.h>

static int flag = 0;

static void diag_func(void) {
#line 100 "assert_diag.c"
    assert(flag == 1);
}

int main(void) {
    diag_func();  /* aborts with the diagnostic; never returns */
    return 0;
}
