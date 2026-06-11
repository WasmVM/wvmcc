/* LANG-6.7.9-11 — designated initializers: `[constant-expression] =` selects
 * an array element, `.identifier =` selects a member; designators may appear
 * out of order, and a non-designated initializer continues from the subobject
 * after the last one designated (C17 6.7.9p6,p7,p17,p18). */
struct S {
    int a;
    int b;
    int c;
};

int main(void) {
    int arr[6] = {[4] = 40, [1] = 10, [2] = 20}; /* out of order */
    if (arr[0] != 0 || arr[1] != 10 || arr[2] != 20) return 1;
    if (arr[3] != 0 || arr[4] != 40 || arr[5] != 0) return 2;

    struct S s = {.c = 3, .a = 1}; /* member designators, out of order */
    if (s.a != 1 || s.b != 0 || s.c != 3) return 3;

    /* Combined: after [2]=5, the next (undesignated) initializer goes to
     * index 3. */
    int mix[5] = {[2] = 5, 6};
    if (mix[0] != 0 || mix[1] != 0 || mix[2] != 5) return 4;
    if (mix[3] != 6 || mix[4] != 0) return 5;

    /* After .b=2, the next initializer goes to member c. */
    struct S t = {.b = 2, 9};
    if (t.a != 0 || t.b != 2 || t.c != 9) return 6;

    /* Nested designators on a member of array-of-struct. */
    struct S grid[3] = {[1].b = 7};
    if (grid[1].b != 7 || grid[1].a != 0 || grid[0].a != 0 || grid[2].c != 0) return 7;

    return 0;
}
