/* tests/standard/libc/float/flt_eval_method.c — FLT_EVAL_METHOD.
 * Catalog: LIBC-float-FLT_EVAL_METHOD-01 (ISO C17 7.7p3, 5.2.4.2.2p9).
 * FLT_EVAL_METHOD characterizes the floating evaluation formats.
 * wvmcc documents 0: evaluate all operations and constants just to the
 * range and precision of the type (matches Wasm f32/f64 arithmetic).
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_EVAL_METHOD == 0,
               "FLT_EVAL_METHOD must be 0 (evaluate to the type)");
