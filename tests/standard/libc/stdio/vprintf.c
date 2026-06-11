/* LIBC-stdio-vprintf-01 — C17 7.21.6.8–7.21.6.14: vprintf / vfprintf /
 * vsnprintf are the va_list variants of the formatted-output functions.
 * Verify=stdout. */
#include <stdarg.h>
#include <stdio.h>

static int vp(const char *fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

static int vfp(FILE *f, const char *fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}

static int vsn(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

int main(void) {
    char buf[8];
    if (vp("%d %s\n", 7, "vp") != 5) return 1;
    if (vfp(stdout, "%s\n", "vfp") != 4) return 2;
    if (vsn(buf, sizeof buf, "%d", 123) != 3) return 3;
    if (puts(buf) < 0) return 4;
    return 0;
}
