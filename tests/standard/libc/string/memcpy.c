/* tests/standard/libc/string/memcpy.c — LIBC-string-memcpy-01. ISO C17 §7.24.2.1. Verify=exit.
 * memcpy copies n bytes from src to dst and returns dst. Small inputs (WasmVM). */
#include <string.h>
int main(void) {
    char dst[6] = {0, 0, 0, 0, 0, 0};
    const char src[6] = {'h', 'e', 'l', 'l', 'o', '\0'};
    if (memcpy(dst, src, 6) != dst) return 1;
    if (dst[0] != 'h' || dst[4] != 'o' || dst[5] != '\0') return 2;
    /* n == 0 copies nothing */
    char keep = dst[0];
    memcpy(dst, "X", 0);
    if (dst[0] != keep) return 3;
    return 0;
}
