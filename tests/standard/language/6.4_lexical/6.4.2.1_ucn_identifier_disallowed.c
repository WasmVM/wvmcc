/* LANG-6.4.2-03 -- constraint violation (C17 6.4.2.1p3): a UCN in an
 * identifier must designate a character allowed by Annex D.1, and the
 * initial character must additionally not fall in the D.2 ranges.
 * \u0040 ('@') is permitted by 6.4.3p2 but is not an identifier
 * character; \u0300 (combining grave accent) is allowed in
 * identifiers but not as the initial character. wvmcc used to accept both
 * silently. */
int \u0040x;
int \u0300y;

int main(void) {
    return 0;
}
