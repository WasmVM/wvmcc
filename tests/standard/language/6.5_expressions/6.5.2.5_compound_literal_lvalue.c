/* LANG-6.5.2.5-01 — A compound literal `( type-name ){ initializer-list }`
 * provides an unnamed object whose value is given by the initializer list, and
 * the result is an lvalue (ISO C17 6.5.2.5p3,p5). */

struct point { int x, y; };

int main(void)
{
    /* The compound literal is an lvalue: we may take its address and modify it. */
    int *p = &(int){ 42 };
    if (*p != 42) return 1;       /* initialized by the list */
    *p = 7;                       /* modifiable lvalue */
    if (*p != 7) return 2;

    /* Struct compound literal value comes from the initializer list. */
    struct point q = (struct point){ .x = 3, .y = 4 };
    if (q.x != 3 || q.y != 4) return 3;

    /* A compound literal of array type is an lvalue that decays to a pointer to
     * its first element; subscripting it is well-defined. */
    if ((int[]){ 10, 20, 30 }[1] != 20) return 4;

    /* Member access on the unnamed object. */
    if ((struct point){ .x = 5, .y = 6 }.y != 6) return 5;

    return 0;
}
