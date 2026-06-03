// #79 e2e — atexit() registers handlers that run in reverse (LIFO) order on
// normal termination, and stdio's flush is routed through atexit.
//
// `main` does its first stdio write before registering handlers, so stdio's
// self-registered flush handler is the first registered and therefore runs
// last — flushing everything the LIFO handler chain produced. Expected stdout:
// "main321".
#include <stdio.h>
#include <stdlib.h>

static void h1(void) { fputs("1", stdout); }
static void h2(void) { fputs("2", stdout); }
static void h3(void) { fputs("3", stdout); }

int main(void) {
    fputs("main", stdout);
    atexit(h1);
    atexit(h2);
    atexit(h3);
    return 0;
}
