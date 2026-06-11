/* LANG-6.5.6-01 — 6.5.6p5 (ISO C17): the result of the binary + operator on two
 * arithmetic operands is their arithmetic sum. Verify=exit: return 0 on
 * success, a distinct non-zero code per failed check. */
int main(void) {
    if (2 + 3 != 5) return 1;
    if (-4 + 4 != 0) return 2;
    int a = 7, b = 35;
    if (a + b != 42) return 3;
    if (0 + 0 != 0) return 4;
    return 0;
}
