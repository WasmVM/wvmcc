/* LANG-6.10.1-04 — the controlling constant expression of a #if must be an
 * integer constant expression (6.10.1p1). A floating constant is not, so a
 * conforming implementation must reject this. Verify=compile-fail. */
#if 1.5
#endif
int main(void) { return 0; }
