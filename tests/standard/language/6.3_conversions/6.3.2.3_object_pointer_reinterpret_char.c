/* LANG-6.3.2.3-07 — 6.3.2.3p7: a pointer to an object type may be converted to
 * a pointer to a different object type. When a pointer to an object is
 * converted to a pointer to a character type, the result points to the lowest
 * addressed byte of the object. Successive increments of the result, up to the
 * size of the object, yield pointers to the remaining bytes of the object. */

int main(void) {
    /* Use an object whose bytes we can identify regardless of endianness. */
    unsigned char buf[4] = { 0x11, 0x22, 0x33, 0x44 };

    /* A pointer to the array object converted to char* points at the lowest
     * byte: it must equal &buf[0]. */
    unsigned char *cp = (unsigned char *)&buf;
    if (cp != &buf[0]) return 1;

    /* Successive increments cover each byte of the object, up to sizeof. */
    for (unsigned i = 0; i < sizeof buf; i++) {
        if (cp[i] != buf[i]) return 2;
        if (&cp[i] != &buf[i]) return 3;
    }

    /* Reinterpret a multi-byte object: an int viewed as bytes. char* points to
     * the lowest byte and increments walk all sizeof(int) bytes. */
    int n = 0;
    unsigned char *np = (unsigned char *)&n;
    if (np != (unsigned char *)&n) return 4;
    for (unsigned i = 0; i < sizeof n; i++) {
        np[i] = (unsigned char)(i + 1);    /* write each byte through char* */
    }
    /* Read back through char* in the same order. */
    for (unsigned i = 0; i < sizeof n; i++) {
        if (np[i] != (unsigned char)(i + 1)) return 5;
    }

    return 0;
}
