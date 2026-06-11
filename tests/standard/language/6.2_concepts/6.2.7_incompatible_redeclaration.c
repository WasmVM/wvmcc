/* LANG-6.2.7-02 — 6.2.7p2: all declarations that refer to the same object or
 * function within one translation unit shall have compatible type; otherwise
 * the behavior is undefined. A conforming compiler diagnoses the violation
 * within a TU (a constraint on redeclaration), so this ill-formed TU must be
 * REJECTED.
 *
 * compile-fail: `count` is declared first as `int`, then redeclared as `long`
 * — two declarations of the same object with incompatible type in one TU.
 */

extern int count;
extern long count;   /* incompatible type for the same identifier — reject */
