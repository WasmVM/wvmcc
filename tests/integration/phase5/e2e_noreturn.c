// _Noreturn codegen: a non-void function whose last statement is a call to a
// no-return function must still validate. Without a trailing `unreachable`,
// control falls through to the function's `end` with an empty operand stack,
// which the Wasm validator rejects ("empty validate value stack").
//
// Covers both propagation paths: a user-declared `_Noreturn` function (die,
// registered as a definition) and the libc `_Noreturn` import (exit). The
// program exits 0 via the no-return path, so the link harness sees success.
#include <stdlib.h>

// User _Noreturn whose own body ends in a no-return call.
_Noreturn void die(int code) { exit(code); }

int main(void) {
    // main returns int, but its last statement never returns. The fix emits an
    // `unreachable` after the call so this is valid.
    die(0);
}
