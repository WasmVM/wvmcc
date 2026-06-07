/* LANG-6.5.2.2-03 — with a prototype in scope, arguments are converted as if by
 * assignment to the parameter types (6.5.2.2p7). Verify=exit. */
static double take_double(double d) { return d; }
static int take_int(int i) { return i; }
static char take_char(char c) { return c; }
int main(void) {
    /* int argument converted to double parameter */
    if (take_double(3) != 3.0) return 1;
    /* double argument converted (truncated as by assignment) to int parameter */
    if (take_int(2.9) != 2) return 2;
    /* int argument converted to char parameter (value preserved for small ints) */
    if (take_char(65) != 'A') return 3;
    /* float argument converted as by assignment to double parameter */
    if (take_double(1.5f) != 1.5) return 4;
    return 0;
}
