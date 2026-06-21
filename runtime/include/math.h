// M2-17: <math.h>.
//
// Minimal subset: IEEE 754 classification and sign manipulation.
// Wasm-native transcendentals (sqrt, sin, cos, …) are deferred until
// the compiler grows an intrinsic mechanism — issue notes the gap.
#ifndef _WVMCC_MATH_H
#define _WVMCC_MATH_H

#define M_PI 3.14159265358979323846
#define M_E  2.71828182845904523536

// 7.12p2 — most-efficient evaluation types. wvmcc has FLT_EVAL_METHOD == 0, so
// they are exactly float and double.
typedef float  float_t;
typedef double double_t;

// NaN / Infinity helpers. wvmcc's constant folder doesn't currently
// evaluate `0.0 / 0.0` (NaN) or `1.0 / 0.0` (+inf) at compile time, so
// file-scope `const double` initializers using those expressions land as
// 0.0. Expose them through tiny runtime functions that construct the
// IEEE 754 bit pattern via memcpy — fully resolved at link time.
double __wvmcc_nan(void);
double __wvmcc_inf(void);
#define NAN      (__wvmcc_nan())
#define INFINITY (__wvmcc_inf())
#define HUGE_VAL  INFINITY
#define HUGE_VALF ((float)INFINITY)
#define HUGE_VALL ((long double)INFINITY)

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

/* 7.12.14 — quiet comparison macros: like the relational operators but never
 * raise "invalid" on a NaN operand (every relation but isunordered is false
 * when either argument is a NaN). */
#define isunordered(x, y)    (isnan(x) || isnan(y))
#define isgreater(x, y)      (!isunordered((x), (y)) && (x) >  (y))
#define isgreaterequal(x, y) (!isunordered((x), (y)) && (x) >= (y))
#define isless(x, y)         (!isunordered((x), (y)) && (x) <  (y))
#define islessequal(x, y)    (!isunordered((x), (y)) && (x) <= (y))
#define islessgreater(x, y)  (!isunordered((x), (y)) && ((x) < (y) || (x) > (y)))

/* 7.12p9 — error-handling capabilities. wvmcc reports math errors via errno
 * (`docs/spec.md`), so math_errhandling == MATH_ERRNO. */
#define MATH_ERRNO       1
#define MATH_ERREXCEPT   2
#define math_errhandling MATH_ERRNO

/* 7.12.12 — fdim/fmax/fmin (a NaN argument is treated as missing data, F.10.9). */
double fmax(double x, double y);
double fmin(double x, double y);
double fdim(double x, double y);

/* 7.12.9 — nearest-integer functions. round() rounds halfway cases away from
 * zero; rint/nearbyint use the default round-to-nearest (ties to even). */
double    ceil(double x);
double    floor(double x);
double    trunc(double x);
double    round(double x);
long      lround(double x);
long long llround(double x);
double    nearbyint(double x);
double    rint(double x);
long      lrint(double x);
long long llrint(double x);

/* 7.12.11 — manipulation functions. */
double nan(const char *tagp);
double nextafter(double x, double y);
double nexttoward(double x, long double y);

#endif // _WVMCC_MATH_H
