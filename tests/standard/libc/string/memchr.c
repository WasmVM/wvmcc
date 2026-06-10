/* tests/standard/libc/string/memchr.c — LIBC-string-memchr-01. ISO C17 §7.24.5.1. Verify=exit.
 * memchr finds the first occurrence of (unsigned char)c in the first n bytes;
 * returns a pointer to it, or NULL if absent. */
#include <string.h>
int main(void) {
    const char buf[5] = {'a', 'b', 'c', 'b', '\0'};
    if (memchr(buf, 'b', 5) != buf + 1) return 1;   /* first occurrence */
    if (memchr(buf, 'z', 5) != NULL) return 2;
    if (memchr(buf, 'c', 2) != NULL) return 3;      /* beyond n not searched */
    if (memchr(buf, '\0', 5) != buf + 4) return 4;  /* null byte findable */
    if (memchr(buf, 'a', 0) != NULL) return 5;      /* n == 0 */
    return 0;
}
