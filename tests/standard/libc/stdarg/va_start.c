/* tests/standard/libc/stdarg/va_start.c — LIBC-stdarg-va_start-01. Verify=exit.
 * C17 7.16.1.4: va_start(ap, parmN) initializes ap so that the first va_arg
 * yields the argument immediately following the last named parameter parmN.
 * Kept tiny for the WasmVM interpreter. */
#include <stdarg.h>

static int first_after(int last, ...) {
    va_list ap;
    va_start(ap, last);
    int v = va_arg(ap, int);
    va_end(ap);
    return v;
}

static int first_after2(int a, int b, ...) {
    /* parmN is the LAST named parameter */
    va_list ap;
    va_start(ap, b);
    int v = va_arg(ap, int);
    va_end(ap);
    return a + b + v;
}

int main(void) {
    if (first_after(10, 42) != 42) return 1;
    if (first_after2(1, 2, 30) != 33) return 2;
    return 0;
}
