/* tests/standard/libc/time/mktime.c — LIBC-time-mktime-01 (C17 7.27.2.3).
 * Verify=exit. mktime converts local broken-down time to time_t, ignoring
 * the original tm_wday/tm_yday, normalizing out-of-range members, and
 * setting tm_wday/tm_yday from the normalized values. Returns (time_t)-1
 * if the time cannot be represented. Small call graph (WasmVM). */
#include <time.h>

int main(void) {
    /* 2000-01-01 12:00:00 local — a Saturday, day-of-year 0. tm_wday and
     * tm_yday are set to garbage; mktime must overwrite them. */
    struct tm t;
    t.tm_sec = 0;  t.tm_min = 0;  t.tm_hour = 12;
    t.tm_mday = 1; t.tm_mon = 0;  t.tm_year = 100;
    t.tm_wday = 99; t.tm_yday = 99; t.tm_isdst = -1;
    if (mktime(&t) == (time_t)-1) return 1;
    if (t.tm_wday != 6) return 2;   /* Sat */
    if (t.tm_yday != 0) return 3;

    /* Normalization: January 32nd becomes February 1st. */
    struct tm n;
    n.tm_sec = 0;  n.tm_min = 0;  n.tm_hour = 12;
    n.tm_mday = 32; n.tm_mon = 0; n.tm_year = 100;
    n.tm_wday = 0; n.tm_yday = 0; n.tm_isdst = -1;
    if (mktime(&n) == (time_t)-1) return 4;
    if (n.tm_mon != 1) return 5;
    if (n.tm_mday != 1) return 6;
    return 0;
}
