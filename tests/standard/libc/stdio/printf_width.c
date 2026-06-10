/* LIBC-stdio-printf-flags-01 — field width, left-justify, zero-pad. Verify=stdout. */
#include <stdio.h>
int main(void) { printf("%5d|%-5d|%05d\n", 7, 7, 7); return 0; }
