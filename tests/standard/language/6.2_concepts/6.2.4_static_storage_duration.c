/* LANG-6.2.4-01 — 6.2.4p3: an object with static storage duration exists for the
 * entire execution of the program and is initialized once before startup. */

static int counter = 41;
int file_scope = 7;

static int bump(void) {
    counter = counter + 1;
    return counter;
}

int main(void) {
    /* File-scope object retains its single-time initialization. */
    if (file_scope != 7) return 1;
    if (counter != 41) return 2;

    /* The same single instance persists across calls. */
    if (bump() != 42) return 3;
    if (bump() != 43) return 4;
    if (counter != 43) return 5;

    return 0;
}
