/* tests/standard/libc/stdarg/va_copy.c — LIBC-stdarg-va_copy-01. Verify=exit.
 * C17 7.16.1.2: va_copy(dest, src) initializes dest as a copy of the current
 * state of src; subsequent va_arg on dest yields the same remaining arguments
 * as on src, and the two traversals are independent. Kept tiny for the WasmVM
 * interpreter. */
#include <stdarg.h>

static int check(int n, ...) {
    va_list ap, cp;
    va_start(ap, n);
    if (va_arg(ap, int) != 1) { va_end(ap); return 1; }
    va_copy(cp, ap); /* copy mid-traversal: both should now see 2 then 3 */
    int a2 = va_arg(ap, int);
    int a3 = va_arg(ap, int);
    int c2 = va_arg(cp, int);
    int c3 = va_arg(cp, int);
    va_end(cp);
    va_end(ap);
    if (a2 != 2 || a3 != 3) return 2;
    if (c2 != 2 || c3 != 3) return 3;
    return 0;
}

int main(void) {
    return check(3, 1, 2, 3);
}
