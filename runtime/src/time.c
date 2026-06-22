// M2-16: <time.h> implementation.

#include <time.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

__attribute__((import_module("sys_proc"), import_name("clock_gettime")))
int sys_proc_clock_gettime(int clk_id, void *ts_ptr);

int clock_gettime(int clk_id, struct timespec *ts) {
    // The host writes an i64 sec + i32 nsec layout. Our struct timespec now
    // uses `long tv_nsec` (C17 7.27.1p4), so stage through a host-layout buffer
    // and widen nsec rather than letting the host write a half-initialized i64.
    struct { time_t sec; int nsec; } host;
    int r = sys_proc_clock_gettime(clk_id, &host);
    if (r < 0) { errno = -r; return -1; }
    ts->tv_sec  = host.sec;
    ts->tv_nsec = host.nsec;
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

clock_t clock(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) return (clock_t)-1;
    // CLOCKS_PER_SEC == 1e6, so report microseconds.
    return (clock_t)(ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
}

int timespec_get(struct timespec *ts, int base) {
    if (base != TIME_UTC) return 0;
    if (clock_gettime(CLOCK_REALTIME, ts) < 0) return 0;
    return base;
}

// ---- civil calendar conversions (Howard Hinnant's algorithms) -----------
// Days are counted from 1970-01-01 (== day 0, a Thursday). `m` is [1,12].

static long days_from_civil(long y, int m, int d) {
    y -= (m <= 2);
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);                    // [0, 399]
    unsigned doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;        // [0, 146096]
    return era * 146097 + (long)doe - 719468;
}

static void civil_from_days(long z, long *y, int *m, int *d) {
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);                 // [0, 146096]
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long yy = (long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);      // [0, 365]
    unsigned mp = (5 * doy + 2) / 153;                           // [0, 11]
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);                    // [1, 31]
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);                       // [1, 12]
    *y = yy + (*m <= 2);
}

static struct tm __tm_buf;
static const char __wday[7][4] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char __mon[12][4] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static struct tm *fill_tm(time_t t, struct tm *out) {
    long days = t / 86400;
    long secs = t % 86400;
    if (secs < 0) { secs += 86400; days -= 1; }
    out->tm_hour = (int)(secs / 3600);
    out->tm_min  = (int)((secs % 3600) / 60);
    out->tm_sec  = (int)(secs % 60);
    long y; int m, d;
    civil_from_days(days, &y, &m, &d);
    out->tm_year = (int)(y - 1900);
    out->tm_mon  = m - 1;
    out->tm_mday = d;
    int wd = (int)((days % 7 + 4) % 7);
    if (wd < 0) wd += 7;
    out->tm_wday = wd;
    out->tm_yday = (int)(days - days_from_civil(y, 1, 1));
    out->tm_isdst = 0;
    return out;
}

struct tm *gmtime(const time_t *timer) {
    if (!timer) return NULL;
    return fill_tm(*timer, &__tm_buf);
}

// wvmcc has no time zones: local time == UTC ("C" locale, 7.27.3).
struct tm *localtime(const time_t *timer) { return gmtime(timer); }

time_t mktime(struct tm *tp) {
    long year = (long)tp->tm_year + 1900;
    int  mon  = tp->tm_mon;
    year += mon / 12;
    mon  %= 12;
    if (mon < 0) { mon += 12; year -= 1; }
    long days = days_from_civil(year, mon + 1, 1) + (tp->tm_mday - 1);
    long secs = days * 86400L
              + (long)tp->tm_hour * 3600L
              + (long)tp->tm_min  * 60L
              + (long)tp->tm_sec;
    fill_tm((time_t)secs, tp);   // normalize the broken-down fields in place
    return (time_t)secs;
}

char *asctime(const struct tm *tp) {
    static char buf[26];
    if (!tp) return NULL;
    sprintf(buf, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
            __wday[tp->tm_wday], __mon[tp->tm_mon], tp->tm_mday,
            tp->tm_hour, tp->tm_min, tp->tm_sec, tp->tm_year + 1900);
    return buf;
}

char *ctime(const time_t *timer) { return asctime(localtime(timer)); }

size_t strftime(char *s, size_t maxsize, const char *fmt,
                const struct tm *tp) {
    char tmp[16];
    size_t i = 0;
    const char *p = fmt;            /* hoisted: wvmcc rejects a qualified-pointer
                                       declaration in a for-init clause */
    for (; *p; p++) {
        const char *out = tmp;
        if (*p != '%') { tmp[0] = *p; tmp[1] = '\0'; }
        else {
            switch (*++p) {
            case 'Y': sprintf(tmp, "%d",   tp->tm_year + 1900); break;
            case 'm': sprintf(tmp, "%.2d", tp->tm_mon + 1);     break;
            case 'd': sprintf(tmp, "%.2d", tp->tm_mday);        break;
            case 'H': sprintf(tmp, "%.2d", tp->tm_hour);        break;
            case 'M': sprintf(tmp, "%.2d", tp->tm_min);         break;
            case 'S': sprintf(tmp, "%.2d", tp->tm_sec);         break;
            case '%': tmp[0] = '%'; tmp[1] = '\0';              break;
            case '\0': p--; continue;                  /* trailing '%' */
            default:  tmp[0] = '%'; tmp[1] = *p; tmp[2] = '\0'; break;
            }
        }
        size_t len = strlen(out);
        if (i + len + 1 > maxsize) return 0;           /* no room (+ NUL) */
        for (size_t k = 0; k < len; k++) s[i++] = out[k];
    }
    if (i + 1 > maxsize) return 0;
    s[i] = '\0';
    return i;
}
