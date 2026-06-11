/* LANG-6.5.2.2-01 — arguments are evaluated and each parameter receives its
 * argument's value (6.5.2.2p4). Verify=exit; returns 0 on pass. */
static int evals;
static int side(int v) { evals++; return v; }
static int sum3(int a, int b, int c) { return a + b + c; }
int main(void) {
    evals = 0;
    if (sum3(side(1), side(2), side(3)) != 6) return 1;
    if (evals != 3) return 2; /* every argument expression was evaluated */
    return 0;
}
