/* LANG-6.5-05 — 6.5p6,p7: the effective type of an object for an access through
 * an lvalue is the declared type; a stored value may be accessed by an lvalue of
 * a compatible type, a qualified version thereof, or a character type.  This
 * test exercises only the *permitted* (well-defined) accesses. */

int main(void) {
    /* Access through the declared (compatible) type. */
    int n = 0x01020304;
    if (n != 0x01020304) return 1;

    /* Access through a qualified version of the compatible type. */
    const int *cp = &n;
    if (*cp != 0x01020304) return 2;

    /* Character-type access of any stored value is always allowed (p7).
     * Sum the bytes; order is impl-defined but the total is endian-invariant. */
    unsigned char *bp = (unsigned char *)&n;
    unsigned int byte_sum = 0;
    for (unsigned i = 0; i < sizeof n; i++)
        byte_sum += bp[i];
    if (byte_sum != (0x01u + 0x02u + 0x03u + 0x04u)) return 3;

    /* Writing through a character lvalue then reading via the declared type:
     * clear the lowest-addressed byte and confirm via byte access. */
    bp[0] = 0;
    if (bp[0] != 0) return 4;

    /* signed/unsigned char are character types: both may alias. */
    signed char *sp = (signed char *)&n;
    if ((unsigned char)sp[1] != bp[1]) return 5;

    return 0;
}
