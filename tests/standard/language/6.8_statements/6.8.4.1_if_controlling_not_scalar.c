/* LANG-6.8.4.1-03 — Constraint: the controlling expression of an `if`
 * statement shall have scalar type (ISO C17 6.8.4.1p1). A structure has no
 * scalar type, so a conforming compiler must reject this. */

struct S { int member; };

int main(void)
{
    struct S s = { 1 };
    if (s) /* error: controlling expression has structure type, not scalar */
        return 1;
    return 0;
}
