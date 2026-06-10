/* LANG-6.7.6.3-03 — parameter type adjustment (C17 6.7.6.3p7,p8): a
 * parameter declared as "array of T" is adjusted to "qualified pointer to
 * T"; a parameter declared as "function returning T" is adjusted to
 * "pointer to function returning T". */

/* "int a[10]" adjusts to "int *a": sizeof sees a pointer, and writes
 * through it reach the caller's array. */
static unsigned long param_size(int a[10]) {
    a[0] = 99;
    return sizeof(a); /* sizeof a pointer, NOT 10*sizeof(int) */
}

/* "int f(int)" as a parameter adjusts to "int (*f)(int)". */
static int apply(int f(int), int v) {
    return f(v);
}

static int twice(int v) { return 2 * v; }

int main(void) {
    int arr[10] = { 0 };

    if (param_size(arr) != sizeof(int *)) return 1;
    if (arr[0] != 99) return 2; /* callee wrote through the adjusted pointer */

    if (apply(twice, 21) != 42) return 3;
    if (apply(&twice, 10) != 20) return 4; /* &f also yields the pointer */

    return 0;
}
