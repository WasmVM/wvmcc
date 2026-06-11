/* LANG-6.2.5-13 — 6.2.5p20: derived types (array, structure, union, function,
 * pointer) are constructible from object/function types, recursively. This
 * exercises each derivation, including nesting (array of struct, pointer to
 * function, pointer to pointer). */

struct point { int x; int y; };

union num { int i; float f; };

/* function type, used through a pointer to function below */
static int add(int a, int b) { return a + b; }

int main(void) {
    /* array type (array of struct) */
    struct point pts[3];
    pts[0].x = 1; pts[0].y = 2;
    pts[2].x = 10; pts[2].y = 20;
    if (pts[0].x + pts[2].y != 21) return 1;

    /* union type */
    union num n;
    n.i = 7;
    if (n.i != 7) return 2;

    /* pointer to object, and pointer to pointer (recursive derivation) */
    int v = 42;
    int *p = &v;
    int **pp = &p;
    if (**pp != 42) return 3;

    /* pointer to function */
    int (*fp)(int, int) = add;
    if (fp(3, 4) != 7) return 4;

    /* array of pointers to struct */
    struct point *parr[2];
    parr[0] = &pts[0];
    parr[1] = &pts[2];
    if (parr[1]->x != 10) return 5;

    return 0;
}
