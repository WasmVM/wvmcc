/* LIBC-stdio-printf-conv-01 — %c, %s, and literal %%. Verify=stdout. */
#include <stdio.h>
int main(void) { printf("%c%c %s%%\n", 'H', 'I', "world"); return 0; }
