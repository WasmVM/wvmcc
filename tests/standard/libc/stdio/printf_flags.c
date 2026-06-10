/* LIBC-stdio-printf-conv-01 — sign flags and %x/%X/%#x/%o. Verify=stdout. */
#include <stdio.h>
int main(void) { printf("%+d % d %x %X %#x %o\n", 1, 1, 255, 255, 255, 8); return 0; }
