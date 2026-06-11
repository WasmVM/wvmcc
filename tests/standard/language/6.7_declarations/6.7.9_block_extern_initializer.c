/* LANG-6.7.9-05 — constraint: if the declaration of an identifier has block
 * scope, and the identifier has external or internal linkage, the declaration
 * shall have no initializer for the identifier (C17 6.7.9p5). */
int main(void) {
    extern int x = 5; /* constraint violation: block-scope extern with initializer */
    return x;
}
