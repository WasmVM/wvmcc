/* LANG-6.7.5-02 — _Alignas placement constraint (C17 6.7.5p2):
 * "An alignment attribute shall not be specified in a declaration of a
 * typedef, or a bit-field, or a function, or a parameter, or an object
 * declared with the register storage-class specifier."  _Alignas in a
 * typedef declaration is a constraint violation a conforming compiler
 * MUST reject. */

typedef _Alignas(8) int aligned_int;

int main(void) {
    aligned_int x = 0;
    return x;
}
