// M2-16: <time.h>.
//
// Wallclock + monotonic time via sys_proc.clock_gettime. Minimal API.
#ifndef _WVMCC_TIME_H
#define _WVMCC_TIME_H

#include <stddef.h>

#define time_t  long
#define clock_t long

// Processor-time resolution. wvmcc reports clock() in microseconds.
#define CLOCKS_PER_SEC ((clock_t)1000000)
// Time base for timespec_get (7.27.1p3): UTC.
#define TIME_UTC 1

struct timespec {
    time_t tv_sec;
    // C17 7.27.1p4 requires `long`. WasmVM's host clock_gettime writes an i32
    // nsec, so clock_gettime() stages through a host-layout buffer and widens
    // it (see time.c) rather than exposing the host's i32 directly.
    long tv_nsec;
};

// Broken-down calendar time (7.27.1p4): the nine required int members.
struct tm {
    int tm_sec;   // seconds [0, 60] (60 for a leap second)
    int tm_min;   // minutes [0, 59]
    int tm_hour;  // hours [0, 23]
    int tm_mday;  // day of month [1, 31]
    int tm_mon;   // months since January [0, 11]
    int tm_year;  // years since 1900
    int tm_wday;  // days since Sunday [0, 6]
    int tm_yday;  // days since January 1 [0, 365]
    int tm_isdst; // Daylight Saving flag
};

#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

time_t  time(time_t *t);
int     clock_gettime(int clk_id, struct timespec *ts);
double  difftime(time_t end, time_t start);
clock_t clock(void);
int     timespec_get(struct timespec *ts, int base);

// Conversions (UTC / "C" locale only — wvmcc has no time zones, 7.27.3).
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
time_t     mktime(struct tm *timeptr);
char      *asctime(const struct tm *timeptr);
char      *ctime(const time_t *timer);
size_t     strftime(char *s, size_t maxsize, const char *format,
                    const struct tm *timeptr);

#endif // _WVMCC_TIME_H
