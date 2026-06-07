/* LANG-6.5.6-02 — 6.5.6p6 (ISO C17): the result of the binary - operator on two
 * arithmetic operands is the difference of the operands (first minus second).
 * Verify=exit. */
int main(void) {
    if (5 - 3 != 2) return 1;
    if (3 - 5 != -2) return 2;
    int a = 42, b = 7;
    if (a - b != 35) return 3;
    if (a - a != 0) return 4;
    return 0;
}
