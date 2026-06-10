/* tests/standard/libc/stdarg/va_end.c — LIBC-stdarg-va_end-01. Verify=exit.
 * C17 7.16.1.3: va_end(ap) ends traversal; after va_end the list may be
 * re-initialized with va_start and traversed again from the beginning
 * (7.16p3: "initialized by ... va_start ... possibly destroyed and
 * reinitialized any number of times"). Kept tiny for the WasmVM interpreter. */
#include <stdarg.h>

static int twice(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int first = va_arg(ap, int);
    va_end(ap);
    /* re-start after va_end: traversal begins again at the first argument */
    va_start(ap, n);
    int again = va_arg(ap, int);
    va_end(ap);
    if (first != 9) return 1;
    if (again != 9) return 2;
    return 0;
}

int main(void) {
    return twice(1, 9);
}
