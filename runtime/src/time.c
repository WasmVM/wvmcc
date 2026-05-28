// M2-16: <time.h> implementation.

#include <time.h>
#include <errno.h>
#include <stddef.h>

__attribute__((import_module("sys_proc"), import_name("clock_gettime")))
int sys_proc_clock_gettime(int clk_id, void *ts_ptr);

int clock_gettime(int clk_id, struct timespec *ts) {
    int r = sys_proc_clock_gettime(clk_id, ts);
    if (r < 0) { errno = -r; return -1; }
    return 0;
}

time_t time(time_t *t) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) return (time_t)-1;
    if (t) *t = ts.tv_sec;
    return ts.tv_sec;
}

double difftime(time_t end, time_t start) {
    return (double)(end - start);
}
