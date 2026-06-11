/* LANG-6.8.5-01 — Constraint: the controlling expression of an iteration
 * statement shall have scalar type (ISO C17 6.8.5p2). A structure has no
 * scalar type, so using a struct value as a `while` controlling expression
 * is a constraint violation a conforming compiler must reject. */

struct S { int x; };

int f(void)
{
    struct S s = { 1 };
    while (s) {     /* ill-formed: controlling expression has struct type */
        return 1;
    }
    return 0;
}
