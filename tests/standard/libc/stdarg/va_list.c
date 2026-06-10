/* tests/standard/libc/stdarg/va_list.c — LIBC-stdarg-va_list-01. Verify=exit.
 * C17 7.16p3: va_list is a complete object type suitable for holding the
 * information needed by va_start/va_arg/va_end/va_copy. Checks it can be
 * declared as an ordinary object and passed to another function (7.16p3
 * explicitly permits passing a va_list as an argument). Kept tiny for the
 * WasmVM interpreter. */
#include <stdarg.h>

static int read_two(va_list ap) {
    int a = va_arg(ap, int);
    int b = va_arg(ap, int);
    return a + b;
}

static int sum2(int n, ...) {
    va_list ap; /* va_list usable as an object type */
    va_start(ap, n);
    int r = read_two(ap);
    va_end(ap);
    return r;
}

int main(void) {
    if (sum2(2, 3, 4) != 7) return 1;
    return 0;
}
