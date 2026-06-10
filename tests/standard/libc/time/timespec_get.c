/* tests/standard/libc/time/timespec_get.c — LIBC-time-timespec_get-01
 * (C17 7.27.2.5). Verify=exit. timespec_get(ts, TIME_UTC) fills *ts with
 * the current calendar time, tv_nsec in [0, 999999999], and returns its
 * base argument on success (zero on failure). Small call graph (WasmVM). */
#include <time.h>

int main(void) {
    struct timespec ts;
    ts.tv_sec = (time_t)-1;
    ts.tv_nsec = -1;

    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) return 1;

    /* 7.27.2.5p3: tv_nsec is set to the integral number of nanoseconds,
     * rounded to the resolution of the system clock. */
    if (ts.tv_nsec < 0) return 2;
    if (ts.tv_nsec > 999999999) return 3;

    /* 7.27.1p4: valid values for tv_sec are >= 0. */
    if (ts.tv_sec < 0) return 4;
    return 0;
}
