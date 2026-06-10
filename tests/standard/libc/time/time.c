/* tests/standard/libc/time/time.c — LIBC-time-time-01 (C17 7.27.2.4).
 * Verify=exit. time() returns the current calendar time, or (time_t)-1 if
 * it is not available; if timer is not NULL the value is also assigned to
 * *timer. Small call graph (WasmVM). */
#include <time.h>

int main(void) {
    /* timer may be a null pointer. */
    time_t t1 = time(NULL);
    if (t1 == (time_t)-1) return 1;

    /* The return value is also assigned to *timer (7.27.2.4p3). */
    time_t stored = (time_t)-1;
    time_t t2 = time(&stored);
    if (t2 == (time_t)-1) return 2;
    if (stored != t2) return 3;
    return 0;
}
