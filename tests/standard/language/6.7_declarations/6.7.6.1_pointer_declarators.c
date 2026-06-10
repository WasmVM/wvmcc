/* LANG-6.7.6.1-01 — pointer declarators (C17 6.7.6.1p1,p2): qualifiers in a
 * pointer declarator qualify the pointer itself, not the pointed-to type:
 *   const int *p      — pointer to const int (pointer modifiable)
 *   int *const q      — const pointer to int (pointee modifiable)
 *   const int *const  — const pointer to const int
 * Pointers to compatible types are compatible (identically qualified). */

int main(void) {
    int a = 1, b = 2;

    /* Pointer to const: the pointer may be reseated. */
    const int *p = &a;
    if (*p != 1) return 1;
    p = &b;
    if (*p != 2) return 2;

    /* Const pointer: the pointee may be modified through it. */
    int *const q = &a;
    *q = 10;
    if (a != 10) return 3;

    /* Const pointer to const. */
    const int *const r = &b;
    if (*r != 2) return 4;

    /* Pointer-type compatibility: int* and int* are compatible; an
     * unqualified pointer value converts to the const-qualified version. */
    int *pa = &a;
    const int *cpa = pa;
    if (*cpa != 10) return 5;

    /* Qualifier between the * and the identifier qualifies the pointer. */
    int *volatile vp = &b;
    *vp = 20;
    if (b != 20) return 6;

    return 0;
}
