// #79 e2e — atexit() handlers run in reverse (LIFO) order on normal
// termination, and stdio's flush is routed through libc's at-exit machinery.
//
// Handlers are registered BEFORE the first stdio write on purpose: stdio's
// flush is a libc-internal at-exit handler that always runs AFTER every user
// handler (matching C's "streams flush after atexit handlers"), so the output
// the LIFO chain produces is still flushed. Expected stdout: "main321".
#include <stdio.h>
#include <stdlib.h>

static void h1(void) { fputs("1", stdout); }
static void h2(void) { fputs("2", stdout); }
static void h3(void) { fputs("3", stdout); }

int main(void) {
    atexit(h1);
    atexit(h2);
    atexit(h3);
    fputs("main", stdout);
    return 0;
}
