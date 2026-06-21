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
