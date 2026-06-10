/* LANG-6.7.9-08 — the initializer for a structure or union object may be a
 * single expression of compatible (struct/union) type; the object takes that
 * value (C17 6.7.9p13). */
struct P {
    int x;
    int y;
};

union U {
    int i;
    long l;
};

static struct P make(void) {
    struct P r;
    r.x = 3;
    r.y = 4;
    return r;
}

int main(void) {
    struct P a = {1, 2};
    struct P b = a; /* initialized from a compatible-type lvalue */
    if (b.x != 1 || b.y != 2) return 1;

    struct P c = make(); /* initialized from a compatible-type rvalue */
    if (c.x != 3 || c.y != 4) return 2;

    union U u;
    u.i = 9;
    union U v = u; /* union copy-initialization */
    if (v.i != 9) return 3;

    return 0;
}
