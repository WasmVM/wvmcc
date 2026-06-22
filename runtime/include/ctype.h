// M2-1: <ctype.h>.
//
// `static inline` implementations covering the ASCII range — no TU; all
// the logic lives in the header and inlines at the call site. Matches the
// C standard semantics for the C locale.
#ifndef _WVMCC_CTYPE_H
#define _WVMCC_CTYPE_H

static inline int isdigit(int c)  { return c >= '0' && c <= '9'; }
static inline int isupper(int c)  { return c >= 'A' && c <= 'Z'; }
static inline int islower(int c)  { return c >= 'a' && c <= 'z'; }
static inline int isalpha(int c)  { return isupper(c) || islower(c); }
static inline int isalnum(int c)  { return isdigit(c) || isalpha(c); }
static inline int isxdigit(int c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline int isspace(int c)  {
    return c == ' ' || c == '\t' || c == '\n'
        || c == '\v' || c == '\f' || c == '\r';
}
static inline int isblank(int c)  { return c == ' ' || c == '\t'; }
static inline int iscntrl(int c)  { return (unsigned)c < 32 || c == 127; }
static inline int isprint(int c)  { return c >= ' ' && c <= '~'; }
static inline int isgraph(int c)  { return c > ' ' && c <= '~'; }
static inline int ispunct(int c)  { return isgraph(c) && !isalnum(c); }

static inline int tolower(int c)  { return isupper(c) ? c + 32 : c; }
static inline int toupper(int c)  { return islower(c) ? c - 32 : c; }

#endif // _WVMCC_CTYPE_H
