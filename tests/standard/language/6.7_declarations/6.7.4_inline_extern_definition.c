/* LANG-6.7.4-03 — inline vs external definition (C17 6.7.4p7):
 * "If all of the file scope declarations for a function in a translation
 * unit include the inline function specifier without extern, then the
 * definition in that translation unit is an inline definition."  Here the
 * function is also declared with `extern`, so the definition below is an
 * EXTERNAL definition: this translation unit provides the one external
 * definition of `twice`, and calls to it must use a real definition. */
extern inline int twice(int n);

inline int twice(int n) { return n * 2; }

int main(void) {
    if (twice(21) != 42) return 1;
    if (twice(0) != 0) return 2;
    if (twice(-5) != -10) return 3;
    return 0;
}
