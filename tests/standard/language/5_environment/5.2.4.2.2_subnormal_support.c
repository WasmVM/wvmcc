/* LANG-5.2.4.2.2-04 — 5.2.4.2.2p10: *_HAS_SUBNORM characterizes whether the
 * type supports subnormal numbers: -1 indeterminable, 0 absent, 1 present.
 * docs/spec.md documents IEEE-754 binary32/64, so subnormals are present. */
#include <float.h>

_Static_assert(FLT_HAS_SUBNORM == 1, "float supports subnormals (IEEE-754 binary32)");
_Static_assert(DBL_HAS_SUBNORM == 1, "double supports subnormals (IEEE-754 binary64)");
_Static_assert(LDBL_HAS_SUBNORM == 1, "long double (== double) supports subnormals");
