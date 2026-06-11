/* LANG-5.2.4.2.2-03 — 5.2.4.2.2p8,p9: FLT_ROUNDS characterizes the rounding
 * mode and FLT_EVAL_METHOD the evaluation format.  docs/spec.md documents
 * round-to-nearest (FLT_ROUNDS == 1) and evaluation exactly in the semantic
 * type (FLT_EVAL_METHOD == 0). */
#include <float.h>

_Static_assert(FLT_ROUNDS == 1, "FLT_ROUNDS == 1 (round to nearest)");
_Static_assert(FLT_EVAL_METHOD == 0, "FLT_EVAL_METHOD == 0 (evaluate in semantic type)");
