// M2-16: <time.h>.
//
// Wallclock + monotonic time via sys_proc.clock_gettime. Minimal API.
#ifndef _WVMCC_TIME_H
#define _WVMCC_TIME_H

#include <stddef.h>

#define time_t long

struct timespec {
    time_t tv_sec;
    // Note: WasmVM's host-side struct uses i32 for nsec. We match that
    // layout exactly to avoid having to stage a copy on every call.
    // POSIX specifies `long`, so this is a documented deviation.
    int tv_nsec;
};

#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

time_t time(time_t *t);
int    clock_gettime(int clk_id, struct timespec *ts);
double difftime(time_t end, time_t start);

#endif // _WVMCC_TIME_H
