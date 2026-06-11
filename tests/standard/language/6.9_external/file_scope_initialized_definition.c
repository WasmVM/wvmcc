/* LANG-6.9.2-01 — a file-scope object declaration with an initializer is an
 * external definition (6.9.2p1). */
int answer = 42;

int main(void) {
    return (answer == 42) ? 0 : 1;
}
