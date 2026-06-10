/* LIBC-stdio-printf-flags-01 — length modifiers %ld / %lld and %u. Verify=stdout. */
#include <stdio.h>
int main(void) { printf("%ld %lld %u\n", 1L, 2LL, 3u); return 0; }
