/* tests/standard/libc/time/types.c — LIBC-time-types-01 (C17 7.27.1).
 * Verify=static-assert. <time.h> declares size_t, clock_t, time_t,
 * struct timespec (time_t tv_sec; long tv_nsec;) and struct tm with at
 * least the nine int members of 7.27.1p4. */
#include <time.h>

/* size_t (7.27.1p2, from 7.19). */
_Static_assert(sizeof(size_t) > 0, "size_t declared");
_Static_assert((size_t)-1 > 0, "size_t is unsigned");

/* clock_t / time_t are (real) types capable of representing times. */
_Static_assert(sizeof(clock_t) > 0, "clock_t declared");
_Static_assert(sizeof(time_t) > 0, "time_t declared");

/* struct timespec: tv_sec has type time_t, tv_nsec has type long (7.27.1p4). */
_Static_assert(_Generic(((struct timespec *)0)->tv_sec, time_t: 1, default: 0),
               "timespec.tv_sec has type time_t");
_Static_assert(_Generic(((struct timespec *)0)->tv_nsec, long: 1, default: 0),
               "timespec.tv_nsec has type long");

/* struct tm: the nine required members, each of type int (7.27.1p4). */
_Static_assert(_Generic(((struct tm *)0)->tm_sec,   int: 1, default: 0), "tm_sec is int");
_Static_assert(_Generic(((struct tm *)0)->tm_min,   int: 1, default: 0), "tm_min is int");
_Static_assert(_Generic(((struct tm *)0)->tm_hour,  int: 1, default: 0), "tm_hour is int");
_Static_assert(_Generic(((struct tm *)0)->tm_mday,  int: 1, default: 0), "tm_mday is int");
_Static_assert(_Generic(((struct tm *)0)->tm_mon,   int: 1, default: 0), "tm_mon is int");
_Static_assert(_Generic(((struct tm *)0)->tm_year,  int: 1, default: 0), "tm_year is int");
_Static_assert(_Generic(((struct tm *)0)->tm_wday,  int: 1, default: 0), "tm_wday is int");
_Static_assert(_Generic(((struct tm *)0)->tm_yday,  int: 1, default: 0), "tm_yday is int");
_Static_assert(_Generic(((struct tm *)0)->tm_isdst, int: 1, default: 0), "tm_isdst is int");
