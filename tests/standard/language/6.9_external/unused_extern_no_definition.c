/* LANG-6.9-03 — an external-linkage identifier not used in an expression needs no
 * definition anywhere in the program (6.9p5). */
extern int never_defined_obj;
extern int never_defined_fn(void);

int main(void) {
    return 0;
}
