/* tests/standard/libc/wchar/types.c — LIBC-wchar-types-01 (C17 7.29.1).
 * Verify=static-assert. <wchar.h> declares the types wchar_t, size_t,
 * mbstate_t, wint_t and struct tm, and defines the macros NULL, WCHAR_MIN,
 * WCHAR_MAX and WEOF (7.29.1p1-p3). All properties below are required of a
 * conforming implementation; checked compile-time only (-ffreestanding). */
#include <wchar.h>

/* The object-like macros must be defined by <wchar.h> (7.29.1p3). */
#ifndef NULL
#error "NULL is not defined by <wchar.h>"
#endif
#ifndef WCHAR_MIN
#error "WCHAR_MIN is not defined by <wchar.h>"
#endif
#ifndef WCHAR_MAX
#error "WCHAR_MAX is not defined by <wchar.h>"
#endif
#ifndef WEOF
#error "WEOF is not defined by <wchar.h>"
#endif

/* wchar_t is an integer type; a wide character constant has type wchar_t
 * (6.4.4.4p11), so they must agree in size. */
_Static_assert(sizeof(L'A') == sizeof(wchar_t),
               "wide character constants have type wchar_t");
_Static_assert((wchar_t)1 == 1, "wchar_t is an integer type");

/* WCHAR_MIN / WCHAR_MAX delimit the range of wchar_t (7.20.3.4):
 * WCHAR_MIN <= -127 or 0, WCHAR_MAX >= 127, and both must be values of
 * type wchar_t (representable, so the casts are identity). */
_Static_assert(WCHAR_MIN <= 0, "WCHAR_MIN is at most 0");
_Static_assert(WCHAR_MAX >= 127, "WCHAR_MAX is at least 127");
_Static_assert((wchar_t)WCHAR_MIN == WCHAR_MIN,
               "WCHAR_MIN is representable in wchar_t");
_Static_assert((wchar_t)WCHAR_MAX == WCHAR_MAX,
               "WCHAR_MAX is representable in wchar_t");
_Static_assert(WCHAR_MIN < WCHAR_MAX, "wchar_t range is non-degenerate");

/* wint_t is an integer type unchanged by the default argument promotions
 * (7.29.1p2): if it were narrower than int all its values would fit in
 * (unsigned) int and it would promote, so sizeof(wint_t) >= sizeof(int). */
_Static_assert(sizeof(wint_t) >= sizeof(int),
               "wint_t is unchanged by default argument promotions");

/* WEOF expands to a constant expression of type wint_t (7.29.1p3). */
_Static_assert((wint_t)WEOF == WEOF, "WEOF is a value of type wint_t");
_Static_assert(sizeof(WEOF) == sizeof(wint_t), "WEOF has type wint_t");

/* size_t is declared by <wchar.h> (7.29.1p2): unsigned integer type. */
_Static_assert((size_t)-1 > 0, "size_t is unsigned");

/* mbstate_t is a complete non-array object type (7.29.1p2). */
static mbstate_t mbstate_obj;
_Static_assert(sizeof(mbstate_obj) > 0, "mbstate_t object is complete");
_Static_assert(sizeof(mbstate_t) > 0, "mbstate_t is a complete object type");

/* struct tm is declared (as an incomplete structure type) by <wchar.h>
 * (7.29.1p2), so a pointer to it can be formed without a definition. */
static struct tm *tm_ptr;
_Static_assert(sizeof(tm_ptr) == sizeof(void *),
               "struct tm is declared; pointers to it can be formed");

/* NULL expands to an implementation-defined null pointer constant
 * (7.19p3): usable as a static initializer for any pointer type. */
static const wchar_t *null_wstr = NULL;
_Static_assert(sizeof(null_wstr) == sizeof(wchar_t *),
               "NULL initializes a pointer at file scope");
