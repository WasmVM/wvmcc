/* LANG-6.2.8-04 — 6.2.8p3: an extended alignment is one greater than
 * _Alignof(max_align_t). Types having an extended alignment are over-aligned
 * types. An object may be given an extended alignment via _Alignas.
 * Verify=static-assert (freestanding).
 *
 * Per docs/spec.md max_align_t has alignment 8, so requesting 16 (or 32) is an
 * extended (over-)alignment beyond the greatest fundamental alignment. The
 * object's _Alignof must report the requested extended value.
 *
 * Compile-only; a held assertion = pass. */
#include <stddef.h>

/* 16 is strictly greater than the greatest fundamental alignment: extended. */
_Alignas(16) static char over16;
_Static_assert(_Alignof(over16) == 16, "_Alignas(16) yields alignment 16");
_Static_assert(_Alignof(over16) > _Alignof(max_align_t),
               "16 is an extended (over-)alignment");

/* A larger extended alignment is also honored and remains a power of two. */
_Alignas(32) static char over32;
_Static_assert(_Alignof(over32) == 32, "_Alignas(32) yields alignment 32");
_Static_assert((_Alignof(over32) & (_Alignof(over32) - 1)) == 0,
               "extended alignment is a power of two");
