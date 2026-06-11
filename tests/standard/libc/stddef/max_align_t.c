/* tests/standard/libc/stddef/max_align_t.c — LIBC-stddef-max_align_t-01 (C17 7.19p2).
 * Verify=static-assert. max_align_t is an object type whose alignment is
 * the greatest fundamental alignment: at least that of every basic type.
 * Catalog status: deferred (not yet in runtime/include/stddef.h) — this
 * test failing to compile is the correct conformance signal. */
#include <stddef.h>

_Static_assert(_Alignof(max_align_t) >= _Alignof(long long),
               "max_align_t alignment >= long long");
_Static_assert(_Alignof(max_align_t) >= _Alignof(double),
               "max_align_t alignment >= double");
_Static_assert(_Alignof(max_align_t) >= _Alignof(void *),
               "max_align_t alignment >= void *");

/* Documented intent for wvmcc: greatest fundamental alignment is 8. */
_Static_assert(_Alignof(max_align_t) == 8, "_Alignof(max_align_t) == 8");
