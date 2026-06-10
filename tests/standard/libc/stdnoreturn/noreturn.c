/* tests/standard/libc/stdnoreturn/noreturn.c — <stdnoreturn.h> noreturn macro.
 * Catalog: LIBC-stdnoreturn-noreturn-01 (docs/standard/libc.md). Verify=exit.
 * ISO C17 7.23p1: <stdnoreturn.h> defines the macro `noreturn`, which expands
 * to `_Noreturn`. 6.7.4p8: a function declared _Noreturn shall not return to
 * its caller. Freestanding-required header (4p6).
 * Returns 0 on success; distinct non-zero per failed check. */
#include <stdnoreturn.h>
#include <stdlib.h>

#ifndef noreturn
#error "7.23p1: noreturn must be defined as a macro by <stdnoreturn.h>"
#endif

/* noreturn must expand to _Noreturn: a redeclaration spelled with the keyword
 * must be compatible with one spelled with the macro (compile-time check). */
noreturn void leave(int code);
_Noreturn void leave(int code);

noreturn void leave(int code)
{
    exit(code);
    /* Control never reaches the closing brace (6.7.4p8). */
}

int main(void)
{
    leave(0); /* success path: terminates the program with status 0 */
    return 2; /* unreachable — reaching here means leave() returned */
}
