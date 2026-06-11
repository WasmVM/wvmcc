/* LANG-6.5.2.2-05 — direct and indirect recursion are permitted (6.5.2.2p11).
 * Verify=exit; returns 0 on pass. */
static int fact(int n) {                 /* direct recursion */
    return n <= 1 ? 1 : n * fact(n - 1);
}

static int is_even(int n);
static int is_odd(int n) {               /* indirect recursion */
    return n == 0 ? 0 : is_even(n - 1);
}
static int is_even(int n) {
    return n == 0 ? 1 : is_odd(n - 1);
}

int main(void) {
    if (fact(5) != 120) return 1;
    if (!is_even(10)) return 2;
    if (is_even(7)) return 3;
    if (!is_odd(7)) return 4;
    return 0;
}
