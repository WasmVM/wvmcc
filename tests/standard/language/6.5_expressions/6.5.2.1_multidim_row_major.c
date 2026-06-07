/* LANG-6.5.2.1-02 — 6.5.2.1p3: for a multidimensional array, successive
 * subscripts select sub-arrays in row-major order; E[i][j] denotes the
 * element at row i, column j, laid out as *((int*)E + i*ncols + j). */

int main(void)
{
    int m[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };

    /* Two-subscript element access. */
    if (m[0][0] != 1) return 1;
    if (m[0][2] != 3) return 2;
    if (m[1][0] != 4) return 3;
    if (m[1][2] != 6) return 4;

    /* m[i] is the i-th sub-array (an int[3]); m[i][j] == *(m[i] + j). */
    if (m[1][1] != *(m[1] + 1)) return 5;

    /* Row-major linear layout: &m[0][0] viewed as a flat array. */
    int *p = &m[0][0];
    if (p[0] != 1) return 6;
    if (p[3] != 4) return 7;   /* start of row 1 follows row 0 */
    if (p[5] != 6) return 8;

    /* E[i][j] == *((int*)m + i*3 + j) */
    if (m[1][2] != *((int *)m + 1 * 3 + 2)) return 9;

    return 0;
}
