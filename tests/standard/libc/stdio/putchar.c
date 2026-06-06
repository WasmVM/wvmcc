/* LIBC-stdio-fputc-01 — putchar writes a single character. Verify=stdout. */
#include <stdio.h>
int main(void) { putchar('A'); putchar('B'); putchar('\n'); return 0; }
