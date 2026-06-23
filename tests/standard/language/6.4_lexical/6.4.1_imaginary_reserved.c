/* LANG-6.4.1-02 — _Imaginary is a reserved keyword (6.4.1p1), so it may not be
 * used as an ordinary identifier. Verify=compile-fail: wvmcc must reject this. */
int _Imaginary = 0;
int main(void) { return 0; }
