/* tests/standard/libc/time/gmtime.c — LIBC-time-gmtime-01 (C17 7.27.3.3,
 * 7.27.3.4). Verify=exit. gmtime converts a calendar time to broken-down
 * UTC time; localtime converts to local time. Both return a pointer to a
 * struct tm with members in their 7.27.1p4 ranges, or NULL on failure.
 * Small call graph (WasmVM). */
#include <time.h>

static int ranges_ok(const struct tm *p) {
    if (p->tm_sec < 0 || p->tm_sec > 60) return 0;   /* leap second */
    if (p->tm_min < 0 || p->tm_min > 59) return 0;
    if (p->tm_hour < 0 || p->tm_hour > 23) return 0;
    if (p->tm_mday < 1 || p->tm_mday > 31) return 0;
    if (p->tm_mon < 0 || p->tm_mon > 11) return 0;
    if (p->tm_wday < 0 || p->tm_wday > 6) return 0;
    if (p->tm_yday < 0 || p->tm_yday > 365) return 0;
    return 1;
}

int main(void) {
    time_t t = time(NULL);
    if (t == (time_t)-1) return 1;

    struct tm *g = gmtime(&t);
    if (g == NULL) return 2;
    if (!ranges_ok(g)) return 3;

    struct tm *l = localtime(&t);
    if (l == NULL) return 4;
    if (!ranges_ok(l)) return 5;
    return 0;
}
