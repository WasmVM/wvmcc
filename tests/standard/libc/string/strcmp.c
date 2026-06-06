/* tests/standard/libc/string/strcmp.c — LIBC-string-strcmp-01. Verify=exit.
 * Minimal slice (see strlen.c note on the WasmVM trap). Checks sign, not value. */
#include <string.h>
int main(void) {
    if (strcmp("wvmcc", "wvmcc") != 0) return 1;
    if (!(strcmp("a", "b") < 0)) return 2;
    if (!(strcmp("b", "a") > 0)) return 3;
    if (!(strcmp("abc", "ab") > 0)) return 4;
    return 0;
}
