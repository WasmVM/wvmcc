/* LANG-6.7.4-01 — inline function specifier (C17 6.7.4p1,p6):
 * A function declared with the `inline` function specifier is an inline
 * function. Making a function an inline function suggests that calls to it
 * be as fast as possible; the suggestion's effectiveness is
 * implementation-defined, but the call semantics are unchanged: the call
 * must behave exactly as a call to a non-inline function. */
static inline int add3(int a, int b, int c) {
    return a + b + c;
}

static inline int square(int n) {
    return n * n;
}

int main(void) {
    if (add3(1, 2, 3) != 6) return 1;
    if (square(7) != 49) return 2;

    /* Inline functions may call other inline functions. */
    if (add3(square(2), square(3), square(4)) != 29) return 3;

    return 0;
}
