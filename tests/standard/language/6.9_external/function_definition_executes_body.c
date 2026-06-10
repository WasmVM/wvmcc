/* LANG-6.9.1-01 — a function definition (declarator + compound statement) executes
 * its body when called (6.9.1p1,p11). */
static int counter = 0;

int bump(void) {
    counter = counter + 1;
    return counter;
}

int main(void) {
    if (bump() != 1) return 1;
    if (bump() != 2) return 2;
    if (counter != 2) return 3;
    return 0;
}
