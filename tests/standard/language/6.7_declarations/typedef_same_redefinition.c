/* LANG-6.7-03 — a typedef name may be redeclared to denote the same type
 * in the same scope (6.7p3). */
typedef int T;
typedef int T; /* OK: same type */

typedef unsigned long size_alias;
typedef unsigned long size_alias; /* OK: same type */

int main(void)
{
    T v = 41;
    size_alias s = 1;
    if (v + (int)s != 42)
        return 1;
    return 0;
}
