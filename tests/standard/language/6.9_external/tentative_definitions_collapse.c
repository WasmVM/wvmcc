/* LANG-6.9.2-02 — multiple tentative definitions of a file-scope object collapse
 * to a single definition; with no explicit definition it is zero-initialized
 * (6.9.2p2). */
int t;
int t;
int t;

int u;
int u = 7; /* an explicit definition overrides the tentative ones */

int main(void) {
    if (t != 0) return 1;
    if (u != 7) return 2;
    return 0;
}
