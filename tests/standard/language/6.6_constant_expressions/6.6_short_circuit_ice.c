/* LANG-6.6-08 — Subexpressions that are not evaluated need not satisfy the
 * constant-expression constraints (ISO C17 6.6p3 wording "contained within
 * a subexpression that is not evaluated", with 6.6p11/footnote latitude):
 * because `||` and `&&` short-circuit, `2 || 1/0` is a valid integer
 * constant expression with value 1 — the division by zero is never
 * evaluated. */

_Static_assert((2 || 1 / 0) == 1, "|| short-circuits: RHS 1/0 not evaluated");
_Static_assert((0 && 1 / 0) == 0, "&& short-circuits: RHS 1/0 not evaluated");

/* The unevaluated arm of `?:` likewise never evaluates its division. */
_Static_assert((1 ? 5 : 1 / 0) == 5, "?: evaluates only the chosen arm");

/* Usable where an ICE is required. */
static int arr[2 || 1 / 0];
_Static_assert(sizeof arr == 1 * sizeof(int), "short-circuit ICE as array size");
