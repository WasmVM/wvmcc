/* LANG-6.5.14-03 — Constraint: each of the operands of `||` shall have scalar
 * type (ISO C17 6.5.14p2). A struct (non-scalar) operand must be rejected by a
 * conforming compiler. */

struct S { int a; };

int f(void)
{
    struct S s = { 0 };
    return s || 1;   /* ill-formed: `||` requires scalar operands */
}
