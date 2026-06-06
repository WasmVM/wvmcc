/* tests/standard/libc/ctype/isdigit.c — LIBC-ctype-isdigit-01. Verify=exit.
 * <ctype.h> is static-inline (no libc TU) — robust on WasmVM. Returns 0 on pass. */
#include <ctype.h>
int main(void) {
    for (int c = '0'; c <= '9'; c++) if (!isdigit(c)) return 1;
    if (isdigit('a') || isdigit('/') || isdigit(':') || isdigit(' ')) return 2;
    return 0;
}
