/* LANG-6.5.2.5-03 — Constraint: the type-name of a compound literal shall
 * specify a complete object type or an array of unknown size, but NOT a
 * variable length array type (ISO C17 6.5.2.5p1). A compound literal whose
 * type-name is a VLA must be rejected. */

int f(int n)
{
    /* Ill-formed: `(int[n]){...}` names a variable length array type, which is
     * not permitted as the type-name of a compound literal. */
    return (int[n]){ 0 }[0];
}
