// <wchar.h> (C17 7.29) — extended multibyte/wide-character utilities.
//
// wvmcc provides the types and macros §7.29.1 requires (the "C" locale is the
// only supported locale); the wide-string/IO functions are not yet implemented.
#ifndef _WVMCC_WCHAR_H
#define _WVMCC_WCHAR_H

#include <stddef.h>   // size_t, wchar_t, NULL

// mbstate_t (shared with <uchar.h>).
#ifndef _WVMCC_MBSTATE_T
#define _WVMCC_MBSTATE_T
typedef struct { unsigned long __count; unsigned int __value; } mbstate_t;
#endif

// wint_t (shared with <wctype.h>): an integer type unchanged by the default
// argument promotions, so at least as wide as int. WEOF is one of its values
// that does not correspond to any wide character.
#ifndef _WVMCC_WINT_T
#define _WVMCC_WINT_T
typedef unsigned int wint_t;
#endif

// struct tm is declared (an incomplete type is sufficient here, 7.29.1p2);
// <time.h> provides the full definition when also included.
struct tm;

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

// wchar_t is `int` in wvmcc (see <stddef.h>): a signed 32-bit type, so its
// range is that of int.
#ifndef WCHAR_MIN
#define WCHAR_MIN (-2147483647 - 1)
#endif
#ifndef WCHAR_MAX
#define WCHAR_MAX 2147483647
#endif

#endif // _WVMCC_WCHAR_H
