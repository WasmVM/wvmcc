/* LANG-6.10.7-01 — a null preprocessing directive (a # with only white space)
 * has no effect (6.10.7). Verify=exit: the TU compiles and runs (exit 0). */
#
int main(void) {
#
    return 0;
}
