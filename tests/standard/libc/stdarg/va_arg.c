/* tests/standard/libc/stdarg/va_arg.c — LIBC-stdarg-va_arg-01. Verify=exit.
 * C17 7.16.1.1: va_arg(ap, type) yields the value of the next argument, with
 * type the post-default-argument-promotion type of the actual argument.
 * char/short promote to int, float promotes to double (6.5.2.2p6). Each call
 * advances ap. Kept tiny for the WasmVM interpreter. */
#include <stdarg.h>

static int check(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int i = va_arg(ap, int);          /* int argument */
    int c = va_arg(ap, int);          /* char argument, promoted to int */
    long l = va_arg(ap, long);        /* long argument */
    double d = va_arg(ap, double);    /* float argument, promoted to double */
    va_end(ap);
    if (i != 7) return 1;
    if (c != 'A') return 2;
    if (l != 100000L) return 3;
    if (d != 1.5) return 4;
    return 0;
}

int main(void) {
    return check(4, 7, (char)'A', 100000L, 1.5f);
}
