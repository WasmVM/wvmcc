// M2-10: <stdlib.h> core implementation.
//
// Process control via sys_proc.exit. Conversion uses a single
// shared strtoul-style accumulator. qsort is plain quicksort
// (median-of-three pivot, insertion sort for tiny partitions);
// bsearch is the iterative textbook form. PRNG is a glibc-style LCG.

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <string.h>

__attribute__((import_module("sys_proc"), import_name("exit")))
_Noreturn void sys_proc_exit(int);

/* C `atexit`: handlers registered here run in reverse order of registration on
   normal termination. The table is fixed-size (C requires at least 32 slots). */
#define _ATEXIT_MAX 32
static void (*__atexit_funcs[_ATEXIT_MAX])(void);
static int   __atexit_count;

int atexit(void (*func)(void)) {
    if (!func || __atexit_count >= _ATEXIT_MAX) return 1; /* table full */
    __atexit_funcs[__atexit_count++] = func;
    return 0;
}

/* A single libc-internal at-exit handler — stdio self-registers its flush here
   on first buffered use (#79). Kept out of the user atexit table so (a) a full
   table can never silently drop the flush, and (b) it always runs *after* every
   user handler, matching C's "streams are flushed after atexit handlers"
   ordering. exit() still references no stdio symbol by name, so a program that
   never touches stdio leaves the hook null and links no flush path. */
static void (*__atexit_libc_hook)(void);

void __atexit_libc(void (*func)(void)) { __atexit_libc_hook = func; }

_Noreturn void exit(int status) {
    /* C: run user atexit handlers in reverse (LIFO) order... */
    while (__atexit_count > 0) {
        __atexit_funcs[--__atexit_count]();
    }
    /* ...then the libc-internal cleanup (stdio flush) after all of them. */
    if (__atexit_libc_hook) __atexit_libc_hook();
    sys_proc_exit(status);
}

/* C: abort() does NOT flush — abnormal termination. */
_Noreturn void abort(void)      { sys_proc_exit(134); }  /* 128 + SIGABRT */

/* C: _Exit() terminates immediately — no atexit handlers, no stream flush. */
_Noreturn void _Exit(int status) { sys_proc_exit(status); }

/* C `at_quick_exit`/`quick_exit` (7.22.4.3, 7.22.4.7): a separate handler
   registry run by quick_exit in reverse order, after which the program
   terminates as if by _Exit (no atexit handlers, no stream flush). */
#define _AT_QUICK_EXIT_MAX 32
static void (*__at_quick_exit_funcs[_AT_QUICK_EXIT_MAX])(void);
static int   __at_quick_exit_count;

int at_quick_exit(void (*func)(void)) {
    if (!func || __at_quick_exit_count >= _AT_QUICK_EXIT_MAX) return 1;
    __at_quick_exit_funcs[__at_quick_exit_count++] = func;
    return 0;
}

_Noreturn void quick_exit(int status) {
    while (__at_quick_exit_count > 0) {
        __at_quick_exit_funcs[--__at_quick_exit_count]();
    }
    sys_proc_exit(status);
}

/* ----- conversion ------------------------------------------------- */

static int is_space(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v'
        || c == '\f' || c == '\r';
}

static int digit_value(int c, int base) {
    int d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
    else return -1;
    if (d >= base) return -1;
    return d;
}

long strtol(const char *s, char **endptr, int base) {
    const char *p = s;
    while (is_space(*p)) p++;
    int neg = 0;
    if (*p == '+') p++;
    else if (*p == '-') { neg = 1; p++; }
    if ((base == 0 || base == 16)
        && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')
        && digit_value(p[2], 16) >= 0) {
        p += 2;
        base = 16;
    } else if (base == 0 && *p == '0') {
        base = 8;
    } else if (base == 0) {
        base = 10;
    }
    unsigned long acc = 0;
    int any = 0;
    int overflow = 0;
    unsigned long cutoff = neg
        ? (unsigned long)(-(LONG_MIN + 1)) + 1
        : (unsigned long)LONG_MAX;
    unsigned long cutlim = cutoff % (unsigned long)base;
    cutoff /= (unsigned long)base;
    for (;;) {
        int d = digit_value(*p, base);
        if (d < 0) break;
        if (overflow
            || acc > cutoff
            || (acc == cutoff && (unsigned long)d > cutlim)) {
            overflow = 1;
        } else {
            acc = acc * (unsigned long)base + (unsigned long)d;
        }
        p++;
        any = 1;
    }
    if (endptr) *endptr = (char *)(any ? p : s);
    if (overflow) {
        errno = ERANGE;
        return neg ? LONG_MIN : LONG_MAX;
    }
    return neg ? -(long)acc : (long)acc;
}

unsigned long strtoul(const char *s, char **endptr, int base) {
    const char *p = s;
    while (is_space(*p)) p++;
    int neg = 0;
    if (*p == '+') p++;
    else if (*p == '-') { neg = 1; p++; }
    if ((base == 0 || base == 16)
        && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')
        && digit_value(p[2], 16) >= 0) {
        p += 2;
        base = 16;
    } else if (base == 0 && *p == '0') {
        base = 8;
    } else if (base == 0) {
        base = 10;
    }
    unsigned long acc = 0;
    int any = 0;
    int overflow = 0;
    unsigned long cutoff = ULONG_MAX / (unsigned long)base;
    unsigned long cutlim = ULONG_MAX % (unsigned long)base;
    for (;;) {
        int d = digit_value(*p, base);
        if (d < 0) break;
        if (overflow
            || acc > cutoff
            || (acc == cutoff && (unsigned long)d > cutlim)) {
            overflow = 1;
        } else {
            acc = acc * (unsigned long)base + (unsigned long)d;
        }
        p++;
        any = 1;
    }
    if (endptr) *endptr = (char *)(any ? p : s);
    if (overflow) {
        errno = ERANGE;
        return ULONG_MAX;
    }
    return neg ? (unsigned long)(-(long)acc) : acc;
}

/* LP64: `long` and `long long` are both 64-bit, so the long-long string
   conversions share the `strtol`/`strtoul` accumulators exactly. */
long long strtoll(const char *s, char **endptr, int base) {
    return (long long)strtol(s, endptr, base);
}
unsigned long long strtoull(const char *s, char **endptr, int base) {
    return (unsigned long long)strtoul(s, endptr, base);
}

int atoi(const char *s) { return (int)strtol(s, (char **)0, 10); }
long atol(const char *s) { return strtol(s, (char **)0, 10); }
long long atoll(const char *s) { return strtoll(s, (char **)0, 10); }

/* C17 7.22.1.3: strtod/strtof/strtold. A shared double-precision decimal
   parser does the work; the float/long-double entry points narrow the result
   (long double is f64 in wvmcc, so its wrapper is an identity convert).

   The mantissa is accumulated as a double and then scaled by 10^exp via binary
   exponentiation. For magnitudes whose mantissa and 10^|exp| are both exactly
   representable (e.g. 2.5 == 25/10, 0.25 == 25/100) the single divide/multiply
   is exact. A magnitude past DBL_MAX scales to ±infinity on its own, which we
   detect to set ERANGE — no HUGE_VAL/inf-helper dependency needed.

   Not yet handled (no standard test exercises them): hex floats (0x1p4),
   inf/nan spellings, and a distinct underflow→ERANGE path. */
static double __strtod_impl(const char *s, char **endptr) {
    const char *p = s;
    while (is_space(*p)) p++;
    int neg = 0;
    if (*p == '+') p++;
    else if (*p == '-') { neg = 1; p++; }

    double mant = 0.0;
    int any = 0;
    int fracDigits = 0;
    while (*p >= '0' && *p <= '9') {
        mant = mant * 10.0 + (double)(*p - '0');
        p++; any = 1;
    }
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            mant = mant * 10.0 + (double)(*p - '0');
            fracDigits++;
            p++; any = 1;
        }
    }
    if (!any) {                       /* no conversion performed */
        if (endptr) *endptr = (char *)s;
        return 0.0;
    }

    int expVal = 0;
    if (*p == 'e' || *p == 'E') {
        const char *pe = p + 1;
        int eneg = 0;
        if (*pe == '+') pe++;
        else if (*pe == '-') { eneg = 1; pe++; }
        if (*pe >= '0' && *pe <= '9') {
            int e = 0;
            while (*pe >= '0' && *pe <= '9') {
                if (e < 100000) e = e * 10 + (*pe - '0');  /* clamp: huge exp
                                                              already overflows */
                pe++;
            }
            expVal = eneg ? -e : e;
            p = pe;                   /* consume the exponent only if valid */
        }
    }
    if (endptr) *endptr = (char *)p;

    int totalExp = expVal - fracDigits;
    double result = mant;
    if (totalExp != 0 && mant != 0.0) {
        int n = totalExp < 0 ? -totalExp : totalExp;
        double base = 10.0, scale = 1.0;
        while (n) {
            if (n & 1) scale *= base;
            base *= base;
            n >>= 1;
        }
        result = totalExp < 0 ? result / scale : result * scale;
    }
    if (neg) result = -result;

    /* A finite double can never exceed DBL_MAX, so an out-of-range magnitude is
       already ±infinity here; just flag it. */
    if (result > DBL_MAX || result < -DBL_MAX) errno = ERANGE;
    return result;
}

double strtod(const char *s, char **endptr) { return __strtod_impl(s, endptr); }
float  strtof(const char *s, char **endptr) { return (float)__strtod_impl(s, endptr); }
long double strtold(const char *s, char **endptr) {
    return (long double)__strtod_impl(s, endptr);
}

double atof(const char *s) { return __strtod_impl(s, (char **)0); }

/* ----- math ------------------------------------------------------- */

int  abs(int n)  { return n < 0 ? -n : n; }
long labs(long n) { return n < 0 ? -n : n; }
long long llabs(long long n) { return n < 0 ? -n : n; }

/* 7.22.6.2: quotient truncates toward zero; quot*denom + rem == numer. C's
   integer division already truncates toward zero, so this is direct. */
div_t   div(int numer, int denom) {
    div_t r; r.quot = numer / denom; r.rem = numer % denom; return r;
}
ldiv_t  ldiv(long numer, long denom) {
    ldiv_t r; r.quot = numer / denom; r.rem = numer % denom; return r;
}
lldiv_t lldiv(long long numer, long long denom) {
    lldiv_t r; r.quot = numer / denom; r.rem = numer % denom; return r;
}

/* ----- qsort / bsearch -------------------------------------------- */

static void swap_bytes(unsigned char *a, unsigned char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char t = a[i];
        a[i] = b[i];
        b[i] = t;
    }
}

static void qsort_impl(unsigned char *base, size_t n, size_t size,
                       int (*cmp)(const void *, const void *)) {
    if (n < 2) return;
    if (n < 8) {
        /* insertion sort */
        for (size_t i = 1; i < n; i++) {
            for (size_t j = i; j > 0; j--) {
                unsigned char *a = base + (j - 1) * size;
                unsigned char *b = base + j * size;
                if (cmp(a, b) <= 0) break;
                swap_bytes(a, b, size);
            }
        }
        return;
    }
    /* median-of-three pivot at the end */
    unsigned char *lo = base;
    unsigned char *hi = base + (n - 1) * size;
    unsigned char *mid = base + (n / 2) * size;
    if (cmp(lo, mid) > 0)  swap_bytes(lo, mid, size);
    if (cmp(lo, hi)  > 0)  swap_bytes(lo, hi,  size);
    if (cmp(mid, hi) > 0)  swap_bytes(mid, hi, size);
    swap_bytes(mid, hi, size);   /* stash pivot at end */
    unsigned char *pivot = hi;
    size_t store = 0;
    for (size_t i = 0; i < n - 1; i++) {
        unsigned char *e = base + i * size;
        if (cmp(e, pivot) < 0) {
            if (i != store) swap_bytes(e, base + store * size, size);
            store++;
        }
    }
    swap_bytes(base + store * size, pivot, size);
    qsort_impl(base, store, size, cmp);
    qsort_impl(base + (store + 1) * size, n - store - 1, size, cmp);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
    qsort_impl((unsigned char *)base, nmemb, size, compar);
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) {
    const unsigned char *lo = (const unsigned char *)base;
    size_t n = nmemb;
    while (n > 0) {
        size_t mid = n / 2;
        const unsigned char *p = lo + mid * size;
        int c = compar(key, p);
        if (c == 0) return (void *)p;
        if (c > 0) { lo = p + size; n -= mid + 1; }
        else       { n = mid; }
    }
    return (void *)0;
}

/* ----- PRNG ------------------------------------------------------- */

static unsigned int __rand_state = 1;

int rand(void) {
    __rand_state = __rand_state * 1103515245 + 12345;
    return (int)((__rand_state >> 16) & RAND_MAX);
}

void srand(unsigned int seed) { __rand_state = seed; }

/* ----- environment (7.22.4.6) ------------------------------------- */

// Host lookup: getenv(name_ptr, name_len, buf_ptr, buf_len) -> i32; writes the
// value (NUL-terminated) into buf and returns its length, or a negative errno
// (-ENOENT not found, -ERANGE buffer too small).
__attribute__((import_module("sys_proc"), import_name("getenv")))
int __sys_getenv(const char *name, unsigned long name_len, char *buf, unsigned long buf_len);

char *getenv(const char *name) {
    static char __getenv_buf[256];
    int r = __sys_getenv(name, strlen(name), __getenv_buf, sizeof __getenv_buf);
    if (r < 0) return (char *)0;          // not found (or value too long)
    return __getenv_buf;
}

/* ----- multibyte / wide conversion (7.22.7, 7.22.8) --------------- */
// "C" locale only: the single-byte encoding is the identity on 0..255, so each
// multibyte character is exactly one byte and there is no shift state.

int mblen(const char *s, size_t n) {
    if (!s) return 0;                     // no state-dependent encodings
    if (n == 0) return -1;
    return *s ? 1 : 0;                    // NUL maps to 0
}

int mbtowc(wchar_t *pwc, const char *s, size_t n) {
    if (!s) return 0;
    if (n == 0) return -1;
    unsigned char c = (unsigned char)*s;
    if (pwc) *pwc = (wchar_t)c;
    return c ? 1 : 0;
}

int wctomb(char *s, wchar_t wc) {
    if (!s) return 0;                     // no state-dependent encodings
    if ((unsigned long)wc > 255) return -1;   // not representable in one byte
    *s = (char)wc;
    return 1;
}

size_t mbstowcs(wchar_t *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n; i++) {
        unsigned char c = (unsigned char)src[i];
        if (dst) dst[i] = (wchar_t)c;
        if (c == 0) return i;             // terminating NUL is not counted
    }
    return i;
}

size_t wcstombs(char *dst, const wchar_t *src, size_t n) {
    size_t i = 0;
    for (; i < n; i++) {
        wchar_t wc = src[i];
        if (dst) dst[i] = (char)wc;
        if (wc == 0) return i;
    }
    return i;
}
