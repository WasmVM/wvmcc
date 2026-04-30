// Phase 2 verification: compound literals and struct ABI
struct Point { int x; int y; };

void use_point(struct Point *p);

void test_compound() {
    use_point(&(struct Point){3, 7});
}

struct Point make_point(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}
