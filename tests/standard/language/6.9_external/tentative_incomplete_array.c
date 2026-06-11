/* LANG-6.9.2-03 — `int i[];` with only tentative definitions: at the end of the
 * translation unit the type completes to one element, zero-initialized (6.9.2p2). */
int i[];

int main(void) {
    return (i[0] == 0) ? 0 : 1;
}
