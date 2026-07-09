/* LANG-6.4.4.4-04 — constraint violation (C17 6.4.4.4p9): the value of an
 * octal or hexadecimal escape sequence must be representable in the
 * corresponding type (unsigned char for an unprefixed constant). \777 is
 * 511 > UCHAR_MAX; wvmcc used to truncate it silently. */
char c = '\777';

int main(void) {
    return 0;
}
