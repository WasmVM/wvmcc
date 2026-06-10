/* tests/standard/libc/time/difftime.c — LIBC-time-difftime-01 (C17 7.27.2.2).
 * Verify=exit. difftime(time1, time0) computes time1 - time0 expressed in
 * seconds, as a double. difftime(t, t) == 0.0 is portable; the exact-value
 * checks rely on wvmcc's documented time_t encoding (seconds, LP64 long —
 * docs/spec.md). Small call graph (WasmVM). */
#include <time.h>

int main(void) {
    time_t a = (time_t)1000;
    time_t b = (time_t)400;

    /* Same instant -> difference is exactly 0 seconds. */
    if (difftime(a, a) != 0.0) return 1;

    /* B-impl (seconds-encoded time_t): 1000 - 400 == 600 seconds. */
    if (difftime(a, b) != 600.0) return 2;

    /* Antisymmetric: time1 - time0 == -(time0 - time1). */
    if (difftime(b, a) != -600.0) return 3;
    return 0;
}
