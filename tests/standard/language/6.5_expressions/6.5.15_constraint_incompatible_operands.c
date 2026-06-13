/* LANG-6.5.15-04 — Constraints on the conditional operator (ISO C17
 * 6.5.15p2). The first operand of `?:` shall have scalar type. A struct/union
 * (non-scalar) controlling operand is a constraint violation that a conforming
 * compiler must reject. Verify=compile-fail.
 *
 * (The companion 6.5.15p3 constraint on the *second/third* operands — e.g. a
 * pointer paired with an arithmetic operand — is a distinct, separately tracked
 * gap and is not exercised here.) */

struct S { int a; };

int f(void)
{
    struct S s;
    /* ill-formed: the controlling operand `s` has non-scalar (struct) type. */
    int r = s ? 1 : 0;
    return r;
}
