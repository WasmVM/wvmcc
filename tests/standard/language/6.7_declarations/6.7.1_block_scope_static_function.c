/* LANG-6.7.1-04 — block-scope function declaration storage class (C17
 * 6.7.1p7): "The declaration of an identifier for a function that has block
 * scope shall have no explicit storage-class specifier other than extern."
 * Declaring a function `static` inside a block is a constraint violation a
 * conforming compiler MUST reject. */
int main(void) {
    static int g(void); /* only extern is allowed here */
    return 0;
}
