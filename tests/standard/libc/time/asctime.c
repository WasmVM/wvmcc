/* tests/standard/libc/time/asctime.c — LIBC-time-asctime-01 (C17 7.27.3.1,
 * 7.27.3.2). Verify=exit. asctime converts broken-down time to the exact
 * 26-byte string of 7.27.3.1p2 ("%.3s %.3s%3d %.2d:%.2d:%.2d %d\n");
 * ctime(t) is equivalent to asctime(localtime(t)). Small inputs (WasmVM). */
#include <time.h>
#include <string.h>

int main(void) {
    /* Thu Jan  1 00:00:00 1970 — wday 4, fully in range. */
    struct tm t;
    t.tm_sec = 0;  t.tm_min = 0;  t.tm_hour = 0;
    t.tm_mday = 1; t.tm_mon = 0;  t.tm_year = 70;
    t.tm_wday = 4; t.tm_yday = 0; t.tm_isdst = 0;

    char *s = asctime(&t);
    if (s == NULL) return 1;
    /* Exact format per 7.27.3.1p2 (note the two spaces before "1"). */
    if (strcmp(s, "Thu Jan  1 00:00:00 1970\n") != 0) return 2;

    /* ctime: 25 characters + NUL, ending in newline. */
    time_t now = time(NULL);
    if (now == (time_t)-1) return 3;
    char *c = ctime(&now);
    if (c == NULL) return 4;
    if (strlen(c) != 25) return 5;
    if (c[24] != '\n') return 6;
    return 0;
}
