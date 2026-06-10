/* LANG-6.7.2.1-09 — an unnamed structure/union member with no tag is an
 * anonymous structure/union; its members are considered members of the
 * containing structure/union (C17 6.7.2.1p13). */
struct outer {
    int a;
    struct {            /* anonymous structure */
        int b;
        int c;
    };
    union {             /* anonymous union */
        int d;
        unsigned int e;
    };
};

int main(void) {
    struct outer o;

    /* members of the anonymous aggregates are accessed directly */
    o.a = 1;
    o.b = 2;
    o.c = 3;
    o.d = 4;

    if (o.a != 1) return 1;
    if (o.b != 2) return 2;
    if (o.c != 3) return 3;
    if (o.d != 4) return 4;

    /* anonymous-union members overlap */
    o.e = 7u;
    if (o.d != 7) return 5;

    /* the anonymous struct's members live inside the containing struct */
    if (!((char *)&o.b >= (char *)&o && (char *)&o.b < (char *)&o + sizeof o))
        return 6;

    return 0;
}
