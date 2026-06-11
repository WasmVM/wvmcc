/* tests/standard/libc/string/strncmp.c — LIBC-string-strncmp-01. ISO C17 §7.24.4.4. Verify=exit.
 * strncmp compares at most n chars (stopping after a null char). Sign only. */
#include <string.h>
int main(void) {
    if (strncmp("abc", "abc", 3) != 0) return 1;
    if (strncmp("abcX", "abcY", 3) != 0) return 2; /* only first n chars */
    if (!(strncmp("abc", "abd", 3) < 0)) return 3;
    if (!(strncmp("abd", "abc", 3) > 0)) return 4;
    if (strncmp("a", "b", 0) != 0) return 5;       /* n == 0 -> equal */
    if (!(strncmp("ab", "abc", 5) < 0)) return 6;  /* stops at terminator */
    return 0;
}
