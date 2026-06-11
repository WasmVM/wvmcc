/* LANG-6.2.7-04 — 6.2.7p3: for two compatible types, a composite type can be
 * constructed: if one is an array of known constant size, the composite is an
 * array of that size; if only one is a function type with a parameter type
 * list (a prototype), the composite has that parameter type list. A later
 * declaration that refers to the composite type carries these properties.
 *
 * exit: returns 0 on success, distinct non-zero on the first failed check.
 */

/* Array composite: the bounded declaration supplies the size to the composite,
 * so `sizeof grid` reflects the 5-element array even though the first
 * declaration left the bound unspecified. */
extern int grid[];          /* incomplete array type */
int grid[5] = { 1, 2, 3, 4, 5 };   /* completes it -> composite is int[5] */

/* Function composite: an unprototyped declaration composed with a prototyped
 * one yields the prototyped parameter type list. */
int add(int, int);          /* prototype */
int add();                  /* no parameter type list; composite keeps proto */

int add(int a, int b) { return a + b; }

int main(void) {
    /* Composite array type is int[5]: 5 * sizeof(int). */
    if (sizeof grid != 5 * sizeof(int)) return 1;
    if (grid[0] != 1) return 2;
    if (grid[4] != 5) return 3;

    /* Composite function type is the prototype; the call type-checks/works. */
    if (add(2, 3) != 5) return 4;

    return 0;
}
