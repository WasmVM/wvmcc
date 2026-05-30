// Flush-at-exit regression: a line-buffered stdout with no trailing newline
// must still be flushed when the program terminates normally (return from
// main). crt0's start wrapper calls libc's __stdio_exit before sys_proc.exit
// to provide this guarantee — without it, "Hello" (no '\n') would be silently
// dropped. The test harness compares captured stdout against the exact bytes.
#include <stdio.h>

int main(void) {
    printf("Hello");   // deliberately no '\n' — relies on flush-at-exit
    return 0;
}
