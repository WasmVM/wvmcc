/* LANG-6.7.2.3-05 — tag scoping (6.7.2.3p4,p5): all declarations of a tag
   with the same scope declare the same type; a declaration of the same tag in
   an inner scope (or a different tag anywhere) declares a distinct type. */

struct tag { int member; };

/* Different tag, same member layout: a distinct type that coexists fine. */
struct other { int member; };

int main(void)
{
    struct tag outer;
    struct other o;
    outer.member = 5;
    o.member = 9;
    if (o.member != 9) return 1;

    {
        /* Defining `struct tag` again in an inner scope is legal precisely
           because it declares a NEW, distinct type hiding the outer one. */
        struct tag { long a; long b; } inner;
        inner.a = 7;
        inner.b = 8;
        if (inner.a != 7 || inner.b != 8) return 2;
        if (sizeof(struct tag) == sizeof(outer)) return 3; /* distinct layout */
    }

    /* After the block the tag again denotes the file-scope type:
       same tag + same scope = same type, so assignment is well-formed. */
    struct tag again;
    again = outer;
    if (again.member != 5) return 4;

    return 0;
}
