/* LANG-6.2.3-01 — 6.2.3p1: a tag name and an ordinary identifier with the same
 * spelling occupy distinct name spaces and coexist in one scope. */
struct foo { int x; };
int foo = 7;

int main(void) {
    struct foo s;
    s.x = 35;
    if (foo != 7) return 1;
    if (s.x != 35) return 2;
    return 0;
}
