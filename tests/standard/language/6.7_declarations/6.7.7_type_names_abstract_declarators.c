/* LANG-6.7.7-01 — type names (abstract declarators) parse and denote the
 * intended type in casts, sizeof, compound literals, and _Generic
 * (C17 6.7.7p2). */
int main(void) {
    /* Cast with a simple type name. */
    double d = 3.9;
    int i = (int)d;
    if (i != 3) return 1;

    /* sizeof with type names containing abstract declarators. */
    if (sizeof(int[4]) != 4 * sizeof(int)) return 2;
    if (sizeof(int *) != sizeof(int *[1])) return 3;
    if (sizeof(int (*)(void)) == 0) return 4; /* pointer-to-function type name */

    /* Compound literal: type name followed by a brace-enclosed initializer. */
    int *p = (int[]){10, 20, 30};
    if (p[0] != 10 || p[1] != 20 || p[2] != 30) return 5;

    /* _Generic association list uses type names. */
    if (_Generic(1L, long: 1, int: 2, default: 3) != 1) return 6;
    if (_Generic((int *)0, int *: 4, default: 5) != 4) return 7;

    return 0;
}
