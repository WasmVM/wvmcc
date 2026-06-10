/* LANG-6.7.6.2-01 — array declarators (C17 6.7.6.2p3,p4): an array declared
 * with a constant size has complete type; an array declared with [] has
 * incomplete type, completed by a later declaration of the same identifier
 * with a specified size. */

extern int g[];          /* incomplete array type here */
int g[4] = { 1, 2, 3, 4 };  /* completes the type */

static int m[3] = { 5, 6, 7 };  /* complete from the start */

int main(void) {
    if (sizeof(g) != 4 * sizeof(int)) return 1;
    if (g[0] != 1 || g[3] != 4) return 2;

    if (sizeof(m) != 3 * sizeof(int)) return 3;
    if (m[2] != 7) return 4;

    /* Multidimensional: element type itself an array type. */
    int mm[2][3] = { {1, 2, 3}, {4, 5, 6} };
    if (sizeof(mm) != 6 * sizeof(int)) return 5;
    if (sizeof(mm[0]) != 3 * sizeof(int)) return 6;
    if (mm[1][2] != 6) return 7;

    return 0;
}
