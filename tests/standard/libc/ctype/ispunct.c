/* tests/standard/libc/ctype/ispunct.c — LIBC-ctype-ispunct-01. Verify=exit. */
#include <ctype.h>
int main(void) {
    if (!ispunct('!') || !ispunct('.') || !ispunct(',') || !ispunct('~')) return 1;
    if (ispunct('a') || ispunct('0') || ispunct(' ') || ispunct('\n')) return 2;
    return 0;
}
