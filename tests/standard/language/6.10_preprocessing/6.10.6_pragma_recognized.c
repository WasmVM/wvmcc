/* LANG-6.10.6-01 — #pragma is recognized; an unrecognized pragma is ignored
 * (6.10.6, 6.10.6p1). Verify=exit: the TU compiles and runs (exit 0). */
#pragma wvmcc_unknown_pragma_xyz
int main(void) { return 0; }
