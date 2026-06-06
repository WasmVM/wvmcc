/* LANG-6.8.4.2-02 — two case labels with the same value in one switch (6.8.4.2p3). */
int f(int x) { switch (x) { case 1: case 1: return 0; } return 1; }
