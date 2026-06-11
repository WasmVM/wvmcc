/* LANG-6.5.1-02 — 6.5.1p3,p4: a constant and a string literal are primary
 * expressions. A string literal has array-of-char type and decays to a pointer
 * to its first element; its contents are accessible. */

int main(void) {
    /* an integer constant is a primary expression with its value */
    if (5 != 5) return 1;

    /* a character constant */
    if ('A' != 65) return 2;

    /* a floating constant */
    if (1.5 + 1.5 != 3.0) return 3;

    /* a string literal: indexing and NUL terminator */
    if ("hello"[0] != 'h') return 4;
    if ("hello"[4] != 'o') return 5;
    if ("hello"[5] != '\0') return 6;

    /* sizeof on the string-literal array includes the NUL */
    if (sizeof "hi" != 3) return 7;

    return 0;
}
