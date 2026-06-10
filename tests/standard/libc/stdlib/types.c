/* tests/standard/libc/stdlib/types.c — LIBC-stdlib-div_t-01 (C17 7.22p3).
 * div_t / ldiv_t / lldiv_t are structure types with quot and rem members of
 * the corresponding integer type; size_t and wchar_t are also declared.
 * Verify=static-assert (compile-only). */
#include <stdlib.h>

static div_t   d;
static ldiv_t  ld;
static lldiv_t lld;
static size_t  sz;
static wchar_t wc;

_Static_assert(sizeof(d.quot) == sizeof(int) && sizeof(d.rem) == sizeof(int),
               "div_t has int quot and int rem (7.22.6.2)");
_Static_assert(sizeof(ld.quot) == sizeof(long) && sizeof(ld.rem) == sizeof(long),
               "ldiv_t has long quot and long rem (7.22.6.2)");
_Static_assert(sizeof(lld.quot) == sizeof(long long) && sizeof(lld.rem) == sizeof(long long),
               "lldiv_t has long long quot and long long rem (7.22.6.2)");
_Static_assert((size_t)-1 > 0, "size_t is an unsigned integer type (7.19p2)");
_Static_assert(sizeof(wchar_t) >= 1, "wchar_t is declared by <stdlib.h> (7.22p2)");

int dummy; /* non-empty TU; silence unused statics in compile-only mode */
