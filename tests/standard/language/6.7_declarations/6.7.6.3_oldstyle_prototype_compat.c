/* LANG-6.7.6.3-04 — old-style vs prototype compatibility (C17
 * 6.7.6.3p14,p15): a function type with an empty parameter list is
 * compatible with a prototyped function type whose parameters are all
 * unaffected by the default argument promotions; a definition with an
 * identifier list is compatible with a prototype under the same condition
 * (comparing the promoted parameter types). */

/* Empty-parentheses declaration, later defined with a prototype. */
int add();
int add(int a, int b) { return a + b; }

/* Prototype declaration, later defined old-style with an identifier list. */
int sub(int, int);
int sub(a, b)
    int a;
    int b;
{ return a - b; }

int main(void) {
    if (add(30, 12) != 42) return 1;
    if (sub(50, 8) != 42) return 2;

    /* Both declarations denote the same (compatible) function type, so a
     * prototyped pointer may hold either. */
    int (*fp)(int, int) = add;
    if (fp(1, 2) != 3) return 3;
    fp = sub;
    if (fp(5, 2) != 3) return 4;

    return 0;
}
