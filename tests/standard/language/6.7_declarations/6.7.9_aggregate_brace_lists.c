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

/* A struct whose member is an *array* rather than a nested struct. This is the
 * shape that regressed: the member-name walk stopped at Identifier declarators,
 * and `arr` is an Array declarator wrapping the identifier, so the member was
 * absent from the field list entirely and its initializer was silently dropped.
 * `len` first, so `arr` also sits at a non-zero offset. */
struct WithArray {
    int len;
    int arr[3];
};

struct WithPointer {
    int  len;
    int *p;
};

static int g_target = 42;

/* Returned by value, so the caller's `struct WithArray r = make();` copies it
 * field by field -- the same member-name walk, in a different code path. */
static struct WithArray make(void) {
    struct WithArray s = {1, {7, 8, 9}};
    return s;
}

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

    /* An array member of a *block-scope* struct. A file-scope one is emitted as
     * a data segment and so takes an entirely different path -- these must be
     * locals to exercise the element stores. */
    struct WithArray a = {1, {7, 8, 9}}; /* nested braces */
    if (a.len != 1) return 7;
    if (a.arr[0] != 7 || a.arr[1] != 8 || a.arr[2] != 9) return 8;

    struct WithArray b = {1, 7, 8, 9}; /* flattened into the member */
    if (b.len != 1) return 9;
    if (b.arr[0] != 7 || b.arr[1] != 8 || b.arr[2] != 9) return 10;

    struct WithArray c = {.arr = {7, 8}}; /* designated; rest zero-filled */
    if (c.len != 0) return 11;
    if (c.arr[0] != 7 || c.arr[1] != 8 || c.arr[2] != 0) return 12;

    struct WithArray d = {1}; /* partial: the whole member zero-fills */
    if (d.arr[0] != 0 || d.arr[1] != 0 || d.arr[2] != 0) return 13;

    /* A pointer member is the same shape -- Pointer declarator wrapping the
     * identifier -- so it was dropped for the same reason. */
    struct WithPointer e = {1, &g_target};
    if (e.len != 1 || e.p != &g_target || *e.p != 42) return 14;

    /* Copy-initialization from a returned struct carrying an array member. */
    struct WithArray r = make();
    if (r.len != 1) return 15;
    if (r.arr[0] != 7 || r.arr[1] != 8 || r.arr[2] != 9) return 16;

    return 0;
}
