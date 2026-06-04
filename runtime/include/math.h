// M2-17: <math.h>.
//
// Minimal subset: IEEE 754 classification and sign manipulation.
// Wasm-native transcendentals (sqrt, sin, cos, …) are deferred until
// the compiler grows an intrinsic mechanism — issue notes the gap.
#ifndef _WVMCC_MATH_H
#define _WVMCC_MATH_H

#define M_PI 3.14159265358979323846
#define M_E  2.71828182845904523536

// NaN / Infinity helpers. wvmcc's constant folder doesn't currently
// evaluate `0.0 / 0.0` (NaN) or `1.0 / 0.0` (+inf) at compile time, so
// file-scope `const double` initializers using those expressions land as
// 0.0. Expose them through tiny runtime functions that construct the
// IEEE 754 bit pattern via memcpy — fully resolved at link time.
double __wvmcc_nan(void);
double __wvmcc_inf(void);
#define NAN      (__wvmcc_nan())
#define INFINITY (__wvmcc_inf())
#define HUGE_VAL INFINITY

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

int __fpclassify(double);
int __isnan(double);
int __isinf(double);
int __isfinite(double);
int __isnormal(double);
int __signbit(double);

#define fpclassify(x) __fpclassify((double)(x))
#define isnan(x)      __isnan((double)(x))
#define isinf(x)      __isinf((double)(x))
#define isfinite(x)   __isfinite((double)(x))
#define isnormal(x)   __isnormal((double)(x))
#define signbit(x)    __signbit((double)(x))

double copysign(double x, double y);
double fabs(double x);

#endif // _WVMCC_MATH_H
