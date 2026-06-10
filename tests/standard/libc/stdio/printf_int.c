/* LIBC-stdio-printf-conv-01 — %d signed decimal (zero, negative, positive). Verify=stdout. */
#include <stdio.h>
int main(void) { printf("%d %d %d\n", 0, -1, 42); return 0; }
