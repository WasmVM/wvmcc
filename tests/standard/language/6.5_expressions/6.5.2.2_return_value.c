/* LANG-6.5.2.2-02 — the value of a function call is its return value (6.5.2.2p5). */
static int answer(void) { return 42; }
static int counter;
static void bump(void) { counter++; } /* void call yields no value */
int main(void) {
    if (answer() != 42) return 1;
    counter = 0;
    bump();
    if (counter != 1) return 2;
    return 0;
}
