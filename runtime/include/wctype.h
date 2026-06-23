// <wctype.h> (C17 7.30) — wide-character classification and mapping.
//
// wvmcc provides the required types and WEOF; the iswctype/towctrans family is
// not yet implemented (only the "C" locale is supported).
#ifndef _WVMCC_WCTYPE_H
#define _WVMCC_WCTYPE_H

// wint_t (shared with <wchar.h>): an integer type unchanged by the default
// argument promotions.
#ifndef _WVMCC_WINT_T
#define _WVMCC_WINT_T
typedef unsigned int wint_t;
#endif

// 7.30.1p2: wctype_t / wctrans_t are scalar types holding locale-specific
// character classification / mapping handles.
typedef unsigned long wctype_t;
typedef unsigned long wctrans_t;

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

#endif // _WVMCC_WCTYPE_H
