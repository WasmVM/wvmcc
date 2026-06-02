// M2-14 e2e (stdout fixture): NaN, ±Inf, and -0.0 in float printf. The
// add_link_stdout_test harness captures stdout and compares it exactly to
// the third argument, so this program prints one space-separated line.
#include <stdio.h>
#include <math.h>

int main(void) {
    printf("%f %f %f", NAN, INFINITY, -INFINITY);
    return 0;
}
