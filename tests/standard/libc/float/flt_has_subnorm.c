/* tests/standard/libc/float/flt_has_subnorm.c — *_HAS_SUBNORM.
 * Catalog: LIBC-float-FLT_HAS_SUBNORM-01 (ISO C17 5.2.4.2.2p10).
 * Characterizes subnormal-number support: -1 indeterminable, 0 absent,
 * 1 present. IEEE-754 binary formats support subnormals => 1.
 * Verify=static-assert; compile-only, a held assertion = pass. */
#include <float.h>

_Static_assert(FLT_HAS_SUBNORM == 1, "FLT_HAS_SUBNORM must be 1 (IEEE-754)");
_Static_assert(DBL_HAS_SUBNORM == 1, "DBL_HAS_SUBNORM must be 1 (IEEE-754)");
_Static_assert(LDBL_HAS_SUBNORM == 1, "LDBL_HAS_SUBNORM must be 1 (IEEE-754)");
