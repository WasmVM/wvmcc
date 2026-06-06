/* LANG-6.2.2-04 — 6.2.2p5: a function identifier declared with no storage-class
 * specifier has external linkage. Such a function defined in this TU is callable
 * from another TU, and a call here through an `extern` declaration reaches the
 * same definition.
 *
 * Single-TU `exit` test: an externally-linked function declared `extern` (the
 * default for functions) is callable and returns the expected value; taking its
 * address yields a usable function pointer to the one definition.
 */

extern int add(int a, int b);     /* external linkage (default for functions) */

int add(int a, int b) {           /* the single external definition */
    return a + b;
}

int main(void) {
    if (add(2, 3) != 5) return 1;

    /* The name denotes one externally-linked function; its address is stable. */
    int (*fp)(int, int) = add;
    if (fp == 0) return 2;
    if (fp(10, 20) != 30) return 3;
    if (fp != add) return 4;

    return 0;
}
