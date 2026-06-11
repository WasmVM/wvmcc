/* LANG-6.5.3.4-03 — 6.5.3.4p1 (ISO C17): the sizeof operator shall not be
 * applied to an expression that has function type or an incomplete type, to the
 * parenthesized name of such a type, or to an expression that designates a
 * bit-field member. Applying sizeof to a function type is a constraint
 * violation that a conforming compiler must reject.
 * Verify=compile-fail. */

void f(void);

unsigned long g(void)
{
    return sizeof(f);   /* ill-formed: sizeof applied to a function type */
}
