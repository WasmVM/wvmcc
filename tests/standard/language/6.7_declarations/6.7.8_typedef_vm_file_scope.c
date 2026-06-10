/* LANG-6.7.8-03 — constraint: if a typedef name denotes a variably modified
 * type, then it shall have block scope (C17 6.7.8p2). A typedef of a VLA type
 * at file scope must be rejected. */
int n = 5;

typedef int Vla[n]; /* constraint violation: variably modified typedef at file scope */

int main(void) {
    return 0;
}
