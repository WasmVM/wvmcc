/* tests/standard/libc/string/strlen.c — LIBC-string-strlen-01. Verify=exit.
 * Minimal slice: the installed WasmVM interpreter can trap on string.c's
 * i64/size_t loops with larger retained call graphs (see tests/integration
 * note), so this keeps inputs short and the call graph tiny. */
#include <string.h>
int main(void) {
    if (strlen("") != 0) return 1;
    if (strlen("a") != 1) return 2;
    if (strlen("abcde") != 5) return 3;
    return 0;
}
