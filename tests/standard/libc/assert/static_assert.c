/* tests/standard/libc/assert/static_assert.c — LIBC-assert-static_assert-01
 * (C17 7.2p3). Verify=static-assert (compile-only, -ffreestanding).
 * <assert.h> must define the macro static_assert which expands to
 * _Static_assert. Compiles iff the macro exists and the assertions hold. */
#include <assert.h>

static_assert(1, "static_assert must expand to _Static_assert");
static_assert(sizeof(char) == 1, "C17 6.5.3.4p4: sizeof(char) is 1");

int dummy; /* keep the TU non-empty */
