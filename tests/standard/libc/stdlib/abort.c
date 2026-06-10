/* tests/standard/libc/stdlib/abort.c — LIBC-stdlib-abort-01 (C17 7.22.4.1).
 * abort causes abnormal termination: it does not return, does not run atexit
 * handlers, and reports UNSUCCESSFUL termination to the host.
 *
 * Verify=exit, INVERTED: a CONFORMING run terminates with a NONZERO status
 * (register with WILL_FAIL / expect non-zero). The atexit handler below calls
 * _Exit(0), so a broken abort that drains atexit handlers — or an abort that
 * returns — yields status 0, which the inverted harness reports as FAIL. */
#include <stdlib.h>

static void must_not_run(void) { _Exit(0); }

int main(void) {
    if (atexit(must_not_run) != 0) return 0; /* status 0 = fail under inversion */
    abort();
    return 0; /* reaching here means abort returned: nonconforming */
}
