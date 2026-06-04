// Flush on explicit exit(): a buffered, newline-less write followed by exit()
// must still reach the output. Unlike a normal return from main (handled by
// crt0 -> __stdio_exit), exit() never returns to crt0, so libc's exit() itself
// calls __stdio_exit before terminating. The harness compares captured stdout.
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("before-exit");   // no '\n'; line buffer not flushed by the write
    exit(0);                 // must flush on the way out
}
