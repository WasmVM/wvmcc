/* tests/standard/libc/time/strftime.c — LIBC-time-strftime-01 (C17 7.27.3.5).
 * Verify=exit. strftime formats broken-down time per the format string
 * ("C" locale), returns the number of characters placed (excluding the
 * NUL), or 0 if the result (including NUL) would not fit. Small inputs
 * (WasmVM). */
#include <time.h>
#include <string.h>

int main(void) {
    /* Mon 2009-06-15 13:45:30, day-of-year 165. */
    struct tm t;
    t.tm_sec = 30;  t.tm_min = 45;  t.tm_hour = 13;
    t.tm_mday = 15; t.tm_mon = 5;   t.tm_year = 109;
    t.tm_wday = 1;  t.tm_yday = 165; t.tm_isdst = 0;

    char buf[32];
    size_t n = strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &t);
    if (n != 19) return 1;
    if (strcmp(buf, "2009-06-15 13:45:30") != 0) return 2;

    /* %% is replaced by a single %. */
    n = strftime(buf, 32, "%%", &t);
    if (n != 1) return 3;
    if (strcmp(buf, "%") != 0) return 4;

    /* "2009" plus the NUL needs 5 bytes; maxsize 4 -> returns 0. */
    char tiny[4];
    if (strftime(tiny, 4, "%Y", &t) != 0) return 5;
    return 0;
}
