// M2-7: <string.h> implementation.
//
// Correctness over performance. `memcpy`/`memset` could be widened to
// i64 stores for aligned bulk, but the byte-loop versions validate
// trivially and are easy to reason about — revisit when a hot path
// shows up. `strdup` introduces a libc-internal dependency on `malloc`
// (M2-11), which the linker pulls in lazily.

#include <string.h>
#include <stddef.h>
#include <errno.h>

extern void *malloc(size_t);

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d == s || n == 0) return dst;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--) d[i - 1] = s[i - 1];
    }
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    unsigned char b = (unsigned char)c;
    for (size_t i = 0; i < n; i++) d[i] = b;
    return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char b = (unsigned char)c;
    for (size_t i = 0; i < n; i++) {
        if (p[i] == b) return (void *)(p + i);
    }
    return (void *)0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

size_t strnlen(const char *s, size_t maxlen) {
    size_t n = 0;
    while (n < maxlen && s[n]) n++;
    return n;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char a = (unsigned char)s1[i];
        unsigned char b = (unsigned char)s2[i];
        if (a != b) return (int)a - (int)b;
        if (a == 0) return 0;
    }
    return 0;
}

char *strcpy(char *dst, const char *src) {
    char *p = dst;
    while ((*p++ = *src++)) {}
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}

char *strcat(char *dst, const char *src) {
    char *p = dst;
    while (*p) p++;
    while ((*p++ = *src++)) {}
    return dst;
}

char *strncat(char *dst, const char *src, size_t n) {
    char *p = dst;
    while (*p) p++;
    size_t i = 0;
    while (i < n && src[i]) { *p++ = src[i++]; }
    *p = 0;
    return dst;
}

char *strchr(const char *s, int c) {
    char b = (char)c;
    char *r = (char *)0;
    while (1) {
        if (*s == b) { r = (char *)s; break; }
        if (*s == 0) break;
        s++;
    }
    return r;
}

char *strrchr(const char *s, int c) {
    char b = (char)c;
    const char *last = (const char *)0;
    while (1) {
        if (*s == b) last = s;
        if (*s == 0) break;
        s++;
    }
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle) {
    if (*needle == 0) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (*n == 0) return (char *)haystack;
    }
    return (char *)0;
}

char *strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* Is the (non-NUL) byte `c` present in the NUL-terminated set? */
static int char_in_set(char c, const char *set) {
    for (; *set; set++) {
        if (*set == c) return 1;
    }
    return 0;
}

size_t strspn(const char *s, const char *accept) {
    const char *p = s;
    while (*p && char_in_set(*p, accept)) p++;
    return (size_t)(p - s);
}

size_t strcspn(const char *s, const char *reject) {
    const char *p = s;
    while (*p && !char_in_set(*p, reject)) p++;
    return (size_t)(p - s);
}

char *strpbrk(const char *s, const char *accept) {
    for (; *s; s++) {
        if (char_in_set(*s, accept)) return (char *)s;
    }
    return (char *)0;
}

/* strtok keeps cross-call state in a file-scope pointer (not thread-safe — the
   standard permits this; wvmcc is single-threaded by design). */
static char *__strtok_save;

char *strtok(char *s, const char *delim) {
    if (s == (char *)0) s = __strtok_save;
    if (s == (char *)0) return (char *)0;
    /* skip leading delimiters */
    while (*s && char_in_set(*s, delim)) s++;
    if (*s == 0) { __strtok_save = (char *)0; return (char *)0; }
    char *tok = s;
    while (*s && !char_in_set(*s, delim)) s++;
    if (*s) { *s = 0; __strtok_save = s + 1; }
    else __strtok_save = (char *)0;
    return tok;
}

char *strerror(int errnum) {
    switch (errnum) {
    case 0:      return "Success";
    case EDOM:   return "Numerical argument out of domain";
    case ERANGE: return "Numerical result out of range";
    case EILSEQ: return "Illegal byte sequence";
    case ENOMEM: return "Cannot allocate memory";
    case EINVAL: return "Invalid argument";
    case ENOENT: return "No such file or directory";
    case EBADF:  return "Bad file descriptor";
    case EACCES: return "Permission denied";
    default:     return "Unknown error";
    }
}
