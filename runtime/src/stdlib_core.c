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
#include <string.h>

__attribute__((import_module("sys_proc"), import_name("exit")))
_Noreturn void sys_proc_exit(int);

/* C `atexit`: handlers registered here run in reverse order of registration on
   normal termination. The table is fixed-size (C requires at least 32 slots).
   stdio self-registers its flush via atexit on first buffered use (#79), so
   exit() no longer references __stdio_exit directly — a program that never
   touches stdio registers nothing and links no flush path. */
#define _ATEXIT_MAX 32
static void (*__atexit_funcs[_ATEXIT_MAX])(void);
static int   __atexit_count;

int atexit(void (*func)(void)) {
    if (!func || __atexit_count >= _ATEXIT_MAX) return 1; /* table full */
    __atexit_funcs[__atexit_count++] = func;
    return 0;
}

_Noreturn void exit(int status) {
    /* C: run atexit handlers in reverse (LIFO) order, then terminate. */
    while (__atexit_count > 0) {
        __atexit_funcs[--__atexit_count]();
    }
    sys_proc_exit(status);
}

/* C: abort() does NOT flush — abnormal termination. */
_Noreturn void abort(void)      { sys_proc_exit(134); }  /* 128 + SIGABRT */

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

int atoi(const char *s) { return (int)strtol(s, (char **)0, 10); }
long atol(const char *s) { return strtol(s, (char **)0, 10); }

/* ----- math ------------------------------------------------------- */

int  abs(int n)  { return n < 0 ? -n : n; }
long labs(long n) { return n < 0 ? -n : n; }

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
