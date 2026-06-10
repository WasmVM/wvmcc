/* tests/standard/libc/stdalign/defined.c — <stdalign.h> feature-test macros.
 * Catalog: LIBC-stdalign-defined-01 (docs/standard/libc.md). Verify=static-assert.
 * ISO C17 7.15p3: __alignas_is_defined and __alignof_is_defined are each
 * defined and expand to the integer constant 1, suitable for use in
 * #if preprocessing directives.
 * Compile-only (-ffreestanding); a held assertion = pass. */
#include <stdalign.h>

#ifndef __alignas_is_defined
#error "__alignas_is_defined not defined"
#endif
#ifndef __alignof_is_defined
#error "__alignof_is_defined not defined"
#endif

/* Suitable for use in #if (7.15p3). */
#if __alignas_is_defined != 1
#error "__alignas_is_defined != 1"
#endif
#if __alignof_is_defined != 1
#error "__alignof_is_defined != 1"
#endif

_Static_assert(__alignas_is_defined == 1, "__alignas_is_defined == 1");
_Static_assert(__alignof_is_defined == 1, "__alignof_is_defined == 1");
