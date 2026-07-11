/* LANG-6.4.3-02 -- constraint violation (C17 6.4.3p2): a UCN shall not
 * specify a character whose short identifier is less than 00A0 (other than
 * 0024 '$', 0040 '@', 0060 '`'), nor one in the surrogate range D800-DFFF.
 * \u0041 ('A') and \uD800 must both be rejected; wvmcc
 * used to accept them silently. */
int \u0041y;
int \uD800z;

int main(void) {
    return 0;
}
