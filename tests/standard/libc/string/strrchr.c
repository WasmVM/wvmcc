/* tests/standard/libc/string/strrchr.c — LIBC-string-strrchr-01. ISO C17 §7.24.5.5. Verify=exit.
 * strrchr finds the LAST occurrence of (char)c; the terminator counts as part
 * of the string. Returns NULL if absent. */
#include <string.h>
int main(void) {
    const char *s = "abcb";
    if (strrchr(s, 'b') != s + 3) return 1;   /* last occurrence */
    if (strrchr(s, 'z') != NULL) return 2;
    if (strrchr(s, '\0') != s + 4) return 3;  /* terminator is findable */
    if (strrchr(s, 'a') != s) return 4;
    return 0;
}
