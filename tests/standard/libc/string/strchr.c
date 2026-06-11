/* tests/standard/libc/string/strchr.c — LIBC-string-strchr-01. ISO C17 §7.24.5.2. Verify=exit.
 * strchr finds the first occurrence of (char)c; the terminator is part of the
 * string, so strchr(s, '\0') points at it. Returns NULL if absent. */
#include <string.h>
int main(void) {
    const char *s = "abcb";
    if (strchr(s, 'b') != s + 1) return 1;    /* first occurrence */
    if (strchr(s, 'z') != NULL) return 2;
    if (strchr(s, '\0') != s + 4) return 3;   /* terminator is findable */
    if (strchr(s, 'a') != s) return 4;
    return 0;
}
