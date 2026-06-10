/* LANG-6.7.9-10 — aggregate initialization with brace-enclosed lists:
 * initializers apply to subobjects in order (current object), nested
 * aggregates may be fully braced or flattened, and partial inner braces
 * close at the matching subobject (C17 6.7.9p17–p20). */
struct Inner {
    int a;
    int b;
};

struct Outer {
    struct Inner in;
    int c;
};

int main(void) {
    int m[2][3] = {{1, 2, 3}, {4, 5, 6}}; /* fully braced */
    int f[2][3] = {1, 2, 3, 4, 5, 6};     /* flattened, row-major order */
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (m[i][j] != i * 3 + j + 1) return 1;
            if (f[i][j] != m[i][j]) return 2;
        }
    }

    /* Partial bracketing: each inner brace initializes one row; the rest of
     * the row is zero-initialized. */
    int p[2][3] = {{1}, {4, 5}};
    if (p[0][0] != 1 || p[0][1] != 0 || p[0][2] != 0) return 3;
    if (p[1][0] != 4 || p[1][1] != 5 || p[1][2] != 0) return 4;

    struct Outer o = {{7, 8}, 9}; /* nested braces */
    if (o.in.a != 7 || o.in.b != 8 || o.c != 9) return 5;

    struct Outer q = {7, 8, 9}; /* without inner braces: in-order */
    if (q.in.a != 7 || q.in.b != 8 || q.c != 9) return 6;

    return 0;
}
