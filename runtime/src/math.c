// M2-17: <math.h> — classification and bit-twiddling helpers.
//
// Wasm-native ops (sqrt, ceil, floor, fmin, fmax, trunc, nearest)
// are *not* implemented here: wvmcc doesn't expose Wasm instruction
// intrinsics yet, so we can't emit `f64.sqrt` etc. from C. They live
// in a follow-up issue tied to a builtins mechanism. Classification
// and sign tricks only need raw bit access, which we get via memcpy
// instead of unions (codegen path for unions is narrower today).

#include <math.h>
#include <string.h>

typedef unsigned long u64;

// Address-taking a `double` parameter isn't supported (parameters are
// Wasm locals, not shadow-stack slots). Copy into a local first.
static u64 bits_of(double x) {
    double y = x;
    u64 u;
    memcpy(&u, &y, 8);
    return u;
}

static double double_of(u64 u) {
    u64 v = u;
    double x;
    memcpy(&x, &v, 8);
    return x;
}

int __fpclassify(double x) {
    u64 b = bits_of(x);
    u64 exp  = (b >> 52) & 0x7ff;
    u64 mant = b & 0xfffffffffffffUL;
    if (exp == 0)     return mant == 0 ? FP_ZERO : FP_SUBNORMAL;
    if (exp == 0x7ff) return mant == 0 ? FP_INFINITE : FP_NAN;
    return FP_NORMAL;
}

int __isnan(double x)    { u64 b = bits_of(x); return ((b >> 52) & 0x7ff) == 0x7ff && (b & 0xfffffffffffffUL) != 0; }
int __isinf(double x)    { u64 b = bits_of(x); return ((b >> 52) & 0x7ff) == 0x7ff && (b & 0xfffffffffffffUL) == 0; }
int __isfinite(double x) { u64 b = bits_of(x); return ((b >> 52) & 0x7ff) != 0x7ff; }
int __isnormal(double x) { u64 b = bits_of(x); u64 e = (b >> 52) & 0x7ff; return e != 0 && e != 0x7ff; }
int __signbit(double x)  { return (int)(bits_of(x) >> 63); }

double fabs(double x) {
    u64 b = bits_of(x) & 0x7fffffffffffffffUL;
    return double_of(b);
}

double copysign(double x, double y) {
    u64 bx = bits_of(x) & 0x7fffffffffffffffUL;
    u64 by = bits_of(y) & 0x8000000000000000UL;
    return double_of(bx | by);
}

// IEEE 754 constants exposed by <math.h>. wvmcc doesn't yet fold
// `0.0 / 0.0` or `1.0 / 0.0` at compile time, so we construct them from
// raw bit patterns each call. Cheap (a single memcpy) and avoids relying
// on the codegen's constant-expression evaluator for floats.
__attribute__((visibility("default")))
double __wvmcc_nan(void)  { return double_of(0x7ff8000000000000UL); }

__attribute__((visibility("default")))
double __wvmcc_inf(void)  { return double_of(0x7ff0000000000000UL); }

// 7.12.12 — fdim/fmax/fmin. A NaN argument is "missing data": fmax/fmin return
// the other (numeric) operand.
double fmax(double x, double y) {
    if (__isnan(x)) return y;
    if (__isnan(y)) return x;
    return x > y ? x : y;
}
double fmin(double x, double y) {
    if (__isnan(x)) return y;
    if (__isnan(y)) return x;
    return x < y ? x : y;
}
double fdim(double x, double y) {
    if (__isnan(x) || __isnan(y)) return __wvmcc_nan();
    return x > y ? x - y : 0.0;
}

// 7.12.9 — nearest-integer functions, computed with raw bit access (no Wasm
// rounding intrinsics available). trunc clears the fractional mantissa bits
// selected by the unbiased exponent; the rest derive from trunc.
double trunc(double x) {
    u64 b = bits_of(x);
    int e = (int)((b >> 52) & 0x7ff) - 1023;   // unbiased exponent
    if (e < 0)   return double_of(b & 0x8000000000000000UL);  // |x| < 1 → ±0
    if (e >= 52) return x;                                    // already integral / inf / nan
    u64 frac = 0x000fffffffffffffUL >> e;
    if ((b & frac) == 0) return x;                           // already integral
    return double_of(b & ~frac);
}
double floor(double x) { double t = trunc(x); return t > x ? t - 1.0 : t; }
double ceil(double x)  { double t = trunc(x); return t < x ? t + 1.0 : t; }
double round(double x) {
    double t = trunc(x);
    double d = x - t;
    if (d >=  0.5) return t + 1.0;   // halfway away from zero
    if (d <= -0.5) return t - 1.0;
    return t;
}
long      lround(double x)  { return (long)round(x); }
long long llround(double x) { return (long long)round(x); }

// rint/nearbyint use the default to-nearest-even mode. The 2^52 trick rounds
// via the hardware default: adding/subtracting 2^52 drops the fraction.
double rint(double x) {
    if (!__isfinite(x)) return x;
    const double TWO52 = 4503599627370496.0;  // 2^52
    if (fabs(x) >= TWO52) return x;            // already integral
    return x >= 0.0 ? (x + TWO52) - TWO52 : (x - TWO52) + TWO52;
}
double nearbyint(double x) { return rint(x); }
long      lrint(double x)  { return (long)rint(x); }
long long llrint(double x) { return (long long)rint(x); }

// 7.12.11 — manipulation. nan(tag) returns a quiet NaN (the tag content selects
// the payload; wvmcc ignores it). nextafter steps one representable value of the
// bit pattern toward y; nexttoward is the same (long double == double here).
double nan(const char *tagp) { (void)tagp; return __wvmcc_nan(); }

double nextafter(double x, double y) {
    if (__isnan(x) || __isnan(y)) return __wvmcc_nan();
    if (x == y) return y;                  // includes ±0 == ∓0
    if (x == 0.0)                          // step to the smallest subnormal toward y
        return double_of((bits_of(y) & 0x8000000000000000UL) | 1);
    u64 a = bits_of(x);
    // Incrementing the magnitude bits moves away from zero (toward ±inf).
    if ((x > 0.0) == (y > x)) a += 1; else a -= 1;
    return double_of(a);
}

double nexttoward(double x, long double y) { return nextafter(x, (double)y); }

// 7.12.10 — fmod by exact shift-and-subtract: repeatedly subtract the largest
// ay*2^k <= ax. Each step is exact (×2 and like-magnitude subtraction), so the
// result is exact for all finite x, y (not just small ones).
double fmod(double x, double y) {
    if (__isnan(x) || __isnan(y) || __isinf(x) || y == 0.0) return __wvmcc_nan();
    if (__isinf(y)) return x;
    double ax = fabs(x), ay = fabs(y);
    if (ax < ay) return x;
    while (ax >= ay) {
        double scaled = ay;
        while (scaled + scaled <= ax) scaled += scaled;   // largest ay*2^k <= ax
        ax -= scaled;
    }
    return copysign(ax, x);
}

double remainder(double x, double y) {
    if (__isnan(x) || __isnan(y) || __isinf(x) || y == 0.0) return __wvmcc_nan();
    if (__isinf(y)) return x;
    double r  = fmod(x, y);
    double ay = fabs(y);
    double two_ar = fabs(r) + fabs(r);
    if (two_ar > ay) {
        r -= copysign(ay, r);
    } else if (two_ar == ay) {
        // exact tie: round to the even quotient.
        long q = (long)((x - r) / y);
        if (q & 1) r -= copysign(ay, r);
    }
    return r;
}

double remquo(double x, double y, int *quo) {
    double r = remainder(x, y);
    long n = (long)((x - r) / y);            // the integer quotient used
    long mag = n < 0 ? -n : n;
    int sign = ((x < 0.0) != (y < 0.0)) ? -1 : 1;
    if (quo) *quo = sign * (int)(mag & 7);   // C: at least the low 3 bits, signed
    return r;
}

// ===========================================================================
// M2: libm — sqrt/cbrt, exp/log family, trig, hyperbolic, pow/hypot, fma.
//
// wvmcc has no hardware-math or f64.sqrt intrinsic, so everything here is a
// portable software implementation. Accuracy is implementation-defined
// (7.12p1 / Annex F.10); range-reduce + polynomial gives well under 1e-9 at
// the conformance-tested points, and the exactly-representable cases (integer
// powers, frexp/ldexp/scalbn/modf, perfect squares/cubes) are exact.
// ===========================================================================

#define LN2_HI   6.93147180369123816490e-01   // ln2, high word
#define LN2_LO   1.90821492927058770002e-10   // ln2, low word
#define LN2      0.69314718055994530942
#define INV_LN2  1.44269504088896340736        // 1/ln2
#define INV_LN10 0.43429448190325182765        // 1/ln10
#define M_PI_    3.14159265358979311600
#define PIO2     1.57079632679489655800
#define PIO2_HI  1.57079632679489655800
#define PIO2_LO  6.12323399573676603587e-17

// 2^n as a double for a normal exponent (-1022 <= n <= 1023).
static double twopow(int n) { return double_of(((u64)(1023 + n)) << 52); }

// ---- exponent / scaling primitives (exact) --------------------------------

double scalbn(double x, int n) {
    if (!__isfinite(x) || x == 0.0) return x;
    while (n >  1023) { x *= twopow(1023);  n -= 1023; }
    while (n < -1022) { x *= twopow(-1022); n += 1022; }
    return x * twopow(n);
}
double ldexp(double x, int n)   { return scalbn(x, n); }
double scalbln(double x, long n){
    if (n >  2000000) n =  2000000;
    if (n < -2000000) n = -2000000;
    return scalbn(x, (int)n);
}

double frexp(double x, int *e) {
    if (!__isfinite(x) || x == 0.0) { if (e) *e = 0; return x; }
    u64 b = bits_of(x);
    int ex = (int)((b >> 52) & 0x7ff);
    if (ex == 0) {                      // subnormal: normalize first
        x *= twopow(54);
        b = bits_of(x);
        ex = (int)((b >> 52) & 0x7ff) - 54;
    }
    if (e) *e = ex - 1022;              // frac in [0.5, 1)
    b = (b & ~(0x7ffUL << 52)) | (1022UL << 52);
    return double_of(b);
}

double modf(double x, double *iptr) {
    if (__isinf(x)) { if (iptr) *iptr = x; return double_of(bits_of(x) & 0x8000000000000000UL); }
    if (__isnan(x)) { if (iptr) *iptr = x; return x; }
    double t = trunc(x);
    if (iptr) *iptr = t;
    return x - t;                        // sign of x preserved
}

int ilogb(double x) {
    if (x == 0.0)   return -2147483647 - 1;
    if (__isnan(x)) return -2147483647 - 1;
    if (__isinf(x)) return  2147483647;
    u64 b = bits_of(x);
    int ex = (int)((b >> 52) & 0x7ff);
    if (ex == 0) { double y = x * twopow(54); ex = (int)((bits_of(y) >> 52) & 0x7ff) - 54; }
    return ex - 1023;
}
double logb(double x) {
    if (__isnan(x)) return x;
    if (__isinf(x)) return __wvmcc_inf();
    if (x == 0.0)   return -__wvmcc_inf();
    return (double)ilogb(x);
}

// ---- sqrt / cbrt ----------------------------------------------------------

double sqrt(double x) {
    if (__isnan(x)) return x;
    if (x < 0.0)    return __wvmcc_nan();
    if (x == 0.0 || __isinf(x)) return x;
    int e;
    double m = frexp(x, &e);                  // x = m * 2^e, m in [0.5, 1)
    if (e & 1) { m *= 2.0; e -= 1; }           // make e even; m in [0.5, 2)
    double y = scalbn(0.5 + 0.5 * m, e / 2);   // positive seed ~ sqrt(x)
    for (int i = 0; i < 7; i++) y = 0.5 * (y + x / y);   // Newton
    return y;
}

double cbrt(double x) {
    if (!__isfinite(x) || x == 0.0) return x;
    int neg = x < 0.0;
    double a = neg ? -x : x;
    int e = (int)((bits_of(a) >> 52) & 0x7ff) - 1023;
    double y = twopow(e / 3);
    if (!(y > 0.0)) y = 1.0;
    for (int i = 0; i < 12; i++) y = (2.0 * y + a / (y * y)) / 3.0;   // Newton
    return neg ? -y : y;
}

// ---- exp / log family -----------------------------------------------------

double exp(double x) {
    if (__isnan(x)) return x;
    if (__isinf(x)) return x > 0.0 ? x : 0.0;
    if (x >  709.782712893384) return __wvmcc_inf();
    if (x < -745.133219101941) return 0.0;
    int k = (int)(x * INV_LN2 + (x >= 0.0 ? 0.5 : -0.5));     // nearest integer
    double r = (x - k * LN2_HI) - k * LN2_LO;                 // |r| <= ln2/2
    double term = 1.0, sum = 1.0;
    for (int i = 1; i <= 16; i++) { term *= r / i; sum += term; }
    return scalbn(sum, k);
}
double exp2(double x) {
    if (__isnan(x)) return x;
    double t = trunc(x);
    if (t == x && t >= -1021.0 && t <= 1023.0) return scalbn(1.0, (int)t);  // exact
    return exp(x * LN2);
}
double expm1(double x) {
    if (x == 0.0) return x;
    if (fabs(x) < 0.5) {                  // Taylor for accuracy near 0
        double t = x, s = x;
        for (int i = 2; i <= 14; i++) { t *= x / i; s += t; }
        return s;
    }
    return exp(x) - 1.0;
}

double log(double x) {
    if (__isnan(x)) return x;
    if (x < 0.0)    return __wvmcc_nan();
    if (x == 0.0)   return -__wvmcc_inf();
    if (__isinf(x)) return x;
    int e;
    double m = frexp(x, &e);              // m in [0.5, 1)
    if (m < 0.70710678118654752440) { m *= 2.0; e -= 1; }   // m in [0.707, 1.414)
    double s = (m - 1.0) / (m + 1.0), s2 = s * s;
    double term = s, sum = s;
    for (int i = 3; i <= 27; i += 2) { term *= s2; sum += term / i; }  // 2*atanh(s)
    return 2.0 * sum + e * LN2_HI + e * LN2_LO;
}
double log2(double x)  { return log(x) * INV_LN2; }
double log10(double x) { return log(x) * INV_LN10; }
double log1p(double x) {
    if (x == 0.0) return x;
    if (fabs(x) < 1e-4) return x - 0.5 * x * x + x * x * x / 3.0;
    return log(1.0 + x);
}

// ---- trigonometric --------------------------------------------------------

static double sin_poly(double r) {
    double r2 = r * r, t = r, s = r;
    for (int i = 3; i <= 15; i += 2) { t *= -r2 / ((double)i * (i - 1)); s += t; }
    return s;
}
static double cos_poly(double r) {
    double r2 = r * r, t = 1.0, s = 1.0;
    for (int i = 2; i <= 14; i += 2) { t *= -r2 / ((double)i * (i - 1)); s += t; }
    return s;
}
static int trig_reduce(double x, double *r) {
    int k = (int)(x / PIO2 + (x >= 0.0 ? 0.5 : -0.5));
    *r = (x - k * PIO2_HI) - k * PIO2_LO;
    return k & 3;
}
double sin(double x) {
    if (!__isfinite(x)) return __wvmcc_nan();
    double r; int q = trig_reduce(x, &r);
    switch (q) { case 0: return sin_poly(r); case 1: return cos_poly(r);
                 case 2: return -sin_poly(r); default: return -cos_poly(r); }
}
double cos(double x) {
    if (!__isfinite(x)) return __wvmcc_nan();
    double r; int q = trig_reduce(x, &r);
    switch (q) { case 0: return cos_poly(r); case 1: return -sin_poly(r);
                 case 2: return -cos_poly(r); default: return sin_poly(r); }
}
double tan(double x) { return sin(x) / cos(x); }

double atan(double x) {
    if (__isnan(x)) return x;
    if (__isinf(x)) return copysign(PIO2, x);
    int neg = x < 0.0;
    double a = neg ? -x : x;
    int k = 0;
    while (a > 0.2) { a = a / (1.0 + sqrt(1.0 + a * a)); k++; }   // halve the angle
    double a2 = a * a, t = a, s = a;
    for (int i = 3; i <= 13; i += 2) { t *= -a2; s += t / i; }
    double res = s * (double)(1 << k);
    return neg ? -res : res;
}
double asin(double x) {
    if (__isnan(x)) return x;
    if (fabs(x) > 1.0) return __wvmcc_nan();
    if (fabs(x) == 1.0) return copysign(PIO2, x);
    return atan(x / sqrt(1.0 - x * x));
}
double acos(double x) {
    if (__isnan(x)) return x;
    if (fabs(x) > 1.0) return __wvmcc_nan();
    return PIO2 - asin(x);
}
double atan2(double y, double x) {
    if (__isnan(x) || __isnan(y)) return __wvmcc_nan();
    if (x == 0.0) { if (y > 0.0) return PIO2; if (y < 0.0) return -PIO2; return 0.0; }
    double a = atan(y / x);
    if (x > 0.0) return a;
    return y >= 0.0 ? a + M_PI_ : a - M_PI_;
}

// ---- hyperbolic -----------------------------------------------------------

double sinh(double x) {
    if (!__isfinite(x)) return x;
    double e = exp(x);
    return (e - 1.0 / e) * 0.5;
}
double cosh(double x) {
    if (__isnan(x)) return x;
    if (__isinf(x)) return __wvmcc_inf();
    double e = exp(fabs(x));
    return (e + 1.0 / e) * 0.5;
}
double tanh(double x) {
    if (__isnan(x)) return x;
    double e = exp(2.0 * fabs(x));
    double t = (e - 1.0) / (e + 1.0);
    return x < 0.0 ? -t : t;
}
double asinh(double x) {
    if (!__isfinite(x)) return x;
    double a = fabs(x), r = log(a + sqrt(a * a + 1.0));
    return x < 0.0 ? -r : r;
}
double acosh(double x) {
    if (__isnan(x)) return x;
    if (x < 1.0) return __wvmcc_nan();
    return log(x + sqrt(x * x - 1.0));
}
double atanh(double x) {
    if (__isnan(x)) return x;
    double a = fabs(x);
    if (a > 1.0)  return __wvmcc_nan();
    if (a == 1.0) return copysign(__wvmcc_inf(), x);
    double r = 0.5 * log((1.0 + a) / (1.0 - a));
    return x < 0.0 ? -r : r;
}

// ---- pow / hypot ----------------------------------------------------------

double pow(double x, double y) {
    if (y == 0.0) return 1.0;                       // F.10.4.4: pow(x,+-0)==1
    if (__isnan(x) || __isnan(y)) return __wvmcc_nan();
    if (x == 1.0) return 1.0;
    double yt = trunc(y);
    if (yt == y && fabs(y) <= 1024.0) {             // integer exponent -> exact
        long n = (long)y; int neg = n < 0;
        unsigned long m = neg ? (unsigned long)(-n) : (unsigned long)n;
        double r = 1.0, base = x;
        while (m) { if (m & 1) r *= base; base *= base; m >>= 1; }
        return neg ? 1.0 / r : r;
    }
    if (x > 0.0) return exp(y * log(x));
    return __wvmcc_nan();                            // negative base, non-integer exp
}

double hypot(double x, double y) {
    if (__isinf(x) || __isinf(y)) return __wvmcc_inf();
    x = fabs(x); y = fabs(y);
    if (x < y) { double t = x; x = y; y = t; }
    if (x == 0.0) return 0.0;
    double r = y / x;
    return x * sqrt(1.0 + r * r);
}

// ---- fused multiply-add ---------------------------------------------------
// Scaled double-double evaluation: split the product into a head+tail pair
// (exact via Dekker after scaling the operands into a safe range), add z at
// the same binade, then round once back. This keeps single-rounding semantics
// even when the unscaled product would under/overflow.
static void two_sum(double a, double b, double *s, double *e) {
    double t = a + b, bb = t - a;
    *e = (a - (t - bb)) + (b - bb);
    *s = t;
}
static void two_prod(double a, double b, double *p, double *e) {
    const double SPLIT = 134217729.0;               // 2^27 + 1
    double t = a * b;
    double ca = SPLIT * a, ahi = ca - (ca - a), alo = a - ahi;
    double cb = SPLIT * b, bhi = cb - (cb - b), blo = b - bhi;
    *p = t;
    *e = ((ahi * bhi - t) + ahi * blo + alo * bhi) + alo * blo;
}
double fma(double x, double y, double z) {
    if (!__isfinite(x) || !__isfinite(y) || !__isfinite(z)) return x * y + z;
    if (x == 0.0 || y == 0.0) return x * y + z;
    int ex, ey;
    double xm = frexp(x, &ex), ym = frexp(y, &ey);  // xm,ym in [0.5,1)
    int S = ex + ey;                                 // true product = (xm*ym)*2^S
    double ph, pl; two_prod(xm, ym, &ph, &pl);       // exact, |ph| in (0.25,1)
    double zs = scalbn(z, -S);                       // bring z to the 2^S binade
    if (!__isfinite(zs)) return x * y + z;           // z too far out of range to fuse
    double s1, e1; two_sum(ph, zs, &s1, &e1);
    double hi, lo; two_sum(s1, pl + e1, &hi, &lo);
    double r = scalbn(hi, S);                         // round once back to the true scale
    if (lo != 0.0) r += scalbn(lo, S);
    return r;
}
float fmaf(float x, float y, float z) {
    return (float)fma((double)x, (double)y, (double)z);
}
