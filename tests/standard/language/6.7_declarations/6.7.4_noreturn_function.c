/* LANG-6.7.4-05 — _Noreturn function specifier (C17 6.7.4p8):
 * "A function declared with a _Noreturn function specifier shall not
 * return to its caller."  `finish` terminates via exit(), so control
 * never comes back to main; the program's exit status must be the value
 * passed to exit(0). If the call somehow returned, main would return 1. */
#include <stdlib.h>

_Noreturn void finish(int code) {
    exit(code);
}

int main(void) {
    finish(0);   /* must not return */
    return 1;    /* unreachable */
}
