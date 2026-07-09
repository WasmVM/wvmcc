/* tests/standard/libc/math/errno_math.c — LIBC-math-errno-01.
 * Verify=exit. C17 7.12.1: with math_errhandling & MATH_ERRNO (wvmcc:
 * math_errhandling == MATH_ERRNO), a domain error sets errno to EDOM and
 * returns NaN; a pole error or overflow sets errno to ERANGE and returns
 * ±HUGE_VAL. Underflow leaves errno untouched here (7.12.1p6 makes that
 * implementation-defined; documented in docs/spec.md). errno is never
 * cleared by a successful call (7.5p3). */
#include <math.h>
#include <errno.h>

static int expect(double got, int wantClass, int wantErrno, int code) {
    /* wantClass: 0 = NaN, 1 = +inf, 2 = -inf, 3 = finite (unchecked value) */
    if (wantClass == 0 && !isnan(got)) return code;
    if (wantClass == 1 && !(isinf(got) && got > 0.0)) return code;
    if (wantClass == 2 && !(isinf(got) && got < 0.0)) return code;
    if (wantClass == 3 && !isfinite(got)) return code;
    if (errno != wantErrno) return code + 1;
    return 0;
}

int main(void)
{
    int r;

    /* --- domain errors: EDOM + NaN --- */
    errno = 0; r = expect(sqrt(-1.0),        0, EDOM, 10); if (r) return r;
    errno = 0; r = expect(log(-1.0),         0, EDOM, 12); if (r) return r;
    errno = 0; r = expect(asin(2.0),         0, EDOM, 14); if (r) return r;
    errno = 0; r = expect(acos(-2.0),        0, EDOM, 16); if (r) return r;
    errno = 0; r = expect(acosh(0.5),        0, EDOM, 18); if (r) return r;
    errno = 0; r = expect(atanh(2.0),        0, EDOM, 20); if (r) return r;
    errno = 0; r = expect(fmod(1.0, 0.0),    0, EDOM, 22); if (r) return r;
    errno = 0; r = expect(remainder(1.0, 0.0), 0, EDOM, 24); if (r) return r;
    errno = 0; r = expect(pow(-1.5, 0.5),    0, EDOM, 26); if (r) return r;

    /* --- pole errors: ERANGE + ±HUGE_VAL --- */
    errno = 0; r = expect(log(0.0),          2, ERANGE, 30); if (r) return r;
    errno = 0; r = expect(log10(0.0),        2, ERANGE, 32); if (r) return r;
    errno = 0; r = expect(atanh(1.0),        1, ERANGE, 34); if (r) return r;
    errno = 0; r = expect(atanh(-1.0),       2, ERANGE, 36); if (r) return r;

    /* --- overflow: ERANGE + ±HUGE_VAL --- */
    errno = 0; r = expect(exp(1000.0),       1, ERANGE, 40); if (r) return r;
    errno = 0; r = expect(pow(10.0, 400.0),  1, ERANGE, 42); if (r) return r;
    errno = 0; r = expect(pow(2.0, 5000.0),  1, ERANGE, 44); if (r) return r;
    errno = 0; r = expect(sinh(1000.0),      1, ERANGE, 46); if (r) return r;
    errno = 0; r = expect(ldexp(1.0, 5000),  1, ERANGE, 48); if (r) return r;
    errno = 0; r = expect(hypot(1.7e308, 1.7e308), 1, ERANGE, 50); if (r) return r;

    /* --- underflow returns 0 and leaves errno untouched --- */
    errno = 0; r = expect(exp(-1000.0),      3, 0, 60); if (r) return r;
    if (exp(-1000.0) != 0.0) return 62;

    /* --- no spurious errno from internal intermediates --- */
    errno = 0;
    if (tanh(1000.0) != 1.0) return 64;      /* internal exp(2000) must not leak ERANGE */
    if (errno != 0) return 65;
    errno = 0;
    if (fma(1e-300, 1e-300, 1.0) != 1.0) return 66;   /* internal binade shift */
    if (errno != 0) return 67;

    /* --- NaN arguments propagate quietly (not a domain error) --- */
    errno = 0;
    (void)fmod(nan(""), 2.0);
    (void)sqrt(nan(""));
    if (errno != 0) return 70;

    /* --- successful calls never clear errno (7.5p3) --- */
    errno = 123;
    if (sqrt(4.0) != 2.0) return 72;
    if (errno != 123) return 73;

    return 0;
}
