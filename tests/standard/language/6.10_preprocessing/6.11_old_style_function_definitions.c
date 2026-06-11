/* LANG-6.11-01 — 6.11.6, 6.11.7: non-prototype (old-style) function
 * declarators and definitions are obsolescent but still part of C17 and
 * must be accepted. An empty-parens declaration declares a function with
 * no parameter information; an identifier-list definition with parameter
 * declarations between the declarator and the body defines its types. */

/* 6.11.6: non-prototype declaration — empty parentheses. */
int add();
int negate();

/* 6.11.7: old-style (identifier-list) definition. Default argument
 * promotions apply at calls, so use promoted types (int). */
int add(a, b)
    int a;
    int b;
{
    return a + b;
}

int negate(x)
    int x;
{
    return -x;
}

/* Old-style definition with an empty identifier list. */
int forty_two()
{
    return 42;
}

int main(void)
{
    if (add(3, 4) != 7)      return 1;
    if (negate(5) != -5)     return 2;
    if (forty_two() != 42)   return 3;
    if (add(add(1, 2), negate(-4)) != 7) return 4;
    return 0;
}
