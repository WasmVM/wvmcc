/* LANG-6.5.2.2-04 — default argument promotions on variadic arguments
 * (6.5.2.2p6): float promotes to double, integer types narrower than int
 * promote to int. Verify=exit; returns 0 on pass. */
#include <stdarg.h>

/* Read a float argument: it was promoted to double, so retrieve as double. */
static double get_promoted_double(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double v = va_arg(ap, double);
    va_end(ap);
    return v;
}

/* char/short arguments promote to int, so retrieve as int. */
static int get_promoted_int(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int v = va_arg(ap, int);
    va_end(ap);
    return v;
}

int main(void) {
    float f = 2.5f;
    if (get_promoted_double(1, f) != 2.5) return 1;

    char c = 'Z';
    if (get_promoted_int(1, c) != 'Z') return 2;

    short s = 300;
    if (get_promoted_int(1, s) != 300) return 3;

    return 0;
}
