/* LANG-6.4.8-02 (floating variant) — constraint violation (C17 6.4.8p4):
 * `0x1.2.3p4q` is a single pp-number (dots, digits, and the q are all part
 * of it) but converts to no valid floating constant: two fraction dots and
 * a bogus suffix. strtod's prefix parse used to fold it to 0x1.2 silently. */
double a = 0x1.2.3p4q;

int main(void) {
    return 0;
}
