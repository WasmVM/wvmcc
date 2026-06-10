/* tests/standard/libc/math/signbit.c — LIBC-math-signbit-01. Verify=exit.
 * C17 7.12.3.6: signbit returns nonzero iff the sign of the argument is
 * negative — including negative zero and negative infinity. */
#include <math.h>
int main(void) {
    if (signbit(1.0)) return 1;
    if (!signbit(-1.0)) return 2;
    if (signbit(0.0)) return 3;
    if (!signbit(-0.0)) return 4;       /* signed zero carries its sign */
    if (!signbit(-INFINITY)) return 5;
    if (signbit(INFINITY)) return 6;
    return 0;
}
