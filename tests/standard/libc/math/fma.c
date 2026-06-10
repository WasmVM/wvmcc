/* tests/standard/libc/math/fma.c — LIBC-math-fma-01. Verify=exit.
 * C17 7.12.13: fma(x,y,z) computes x*y + z as one ternary operation,
 * rounding once. The third check distinguishes a true fused result from
 * a separate multiply-then-add: 0x1p-537 * 0x1p-538 = 2^-1075 underflows
 * to 0 when rounded alone (tie to even), so naive evaluation yields
 * -0x1p-1074, while a single rounding of 2^-1075 - 2^-1074 = -2^-1075
 * ties to (even) zero. */
#include <math.h>
int main(void) {
    if (fma(2.0, 3.0, 4.0) != 10.0) return 1;
    if (fma(-2.0, 3.0, 6.0) != 0.0) return 2;
    if (fma(0x1p-537, 0x1p-538, -0x1p-1074) != 0.0) return 3; /* fused */
    if (fmaf(2.0f, 3.0f, 4.0f) != 10.0f) return 4;
    return 0;
}
