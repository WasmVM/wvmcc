/* LANG-6.4.4.4-04 (hex variant) — constraint violation (C17 6.4.4.4p9): a
 * hexadecimal escape sequence has no digit-count limit, so its value must be
 * checked against the corresponding type (unsigned char for an unprefixed
 * constant). \x1FF is 511 > UCHAR_MAX. */
char c = '\x1FF';

int main(void) {
    return 0;
}
