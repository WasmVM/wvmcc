// M2-L4 e2e — link a user TU against libc.a through the integrated linker,
// which lazily pulls only the libc TUs needed (string.c + its transitive
// deps) and runs the linked binary under wasmvm. Exercises the full link
// pipeline end-to-end: merge → resolve → crt0 → dead-code → lazy archive pull.
//
// Kept to a single returning expression over strlen/strcmp: the installed
// WasmVM interpreter has an out-of-scope bug that traps/segfaults on some of
// string.c's i64 (size_t) loops when larger call graphs are retained, so this
// test deliberately exercises a minimal, robust slice. The point under test is
// the *linker* (lazy pull + correct multi-module merge/crt0/resolve), not the
// breadth of libc.
#include <string.h>

int main(void) {
    // strlen("abcde") == 5  →  (5 - 5) == 0;  strcmp(eq) == 0.  Sum 0 on pass.
    return (int)(strlen("abcde") - 5) + strcmp("wvmcc", "wvmcc");
}
