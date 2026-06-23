/* LANG-6.4.2.2-01 — __func__ is implicitly declared as a static array holding
 * the enclosing function's name (6.4.2.2p1). Verify=stdout. */
#include <stdio.h>
static void greet(void) { printf("%s\n", __func__); }
int main(void) { greet(); return 0; }
