/* tests/standard/libc/time/clock.c — LIBC-time-clock-01 (C17 7.27.2.1).
 * Verify=exit. clock() returns the processor time used, or (clock_t)-1 if
 * unavailable; dividing by CLOCKS_PER_SEC yields seconds (7.27.2.1p3).
 * Small call graph (WasmVM). */
#include <time.h>

int main(void) {
    clock_t c1 = clock();
    if (c1 == (clock_t)-1) return 1;

    /* clock()/CLOCKS_PER_SEC is the elapsed processor time in seconds;
     * it cannot be negative. */
    double s1 = (double)c1 / CLOCKS_PER_SEC;
    if (s1 < 0.0) return 2;

    /* Processor time used does not decrease across calls. */
    clock_t c2 = clock();
    if (c2 == (clock_t)-1) return 3;
    if ((double)c2 / CLOCKS_PER_SEC < s1) return 4;
    return 0;
}
