/* LANG-6.5.6-12 — 6.5.6p3 (ISO C17): for subtraction, both operands must be
 * pointers to qualified or unqualified versions of compatible complete object
 * types. Subtracting pointers to incompatible object types (int* and double*)
 * violates this constraint and must be rejected. Verify=compile-fail. */
#include <stddef.h>
ptrdiff_t f(int *p, double *q) {
    return p - q;   /* incompatible pointer types subtracted */
}
