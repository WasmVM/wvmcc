/* LANG-6.8.6.1-03 — Two forward `goto`s whose goto→label ranges overlap
 * without nesting are still plain unconditional jumps (ISO C17 6.8.6.1p2);
 * the ranges [goto A, A:] and [goto B, B:] interleave, which the structural
 * Block lift cannot express, so codegen must fall back to the dispatch
 * loop (#109). */

int main(void)
{
    int trace = 0;

    goto A;
    trace |= 1;             /* skipped by goto A */
    goto B;                 /* range overlaps [goto A, A:] */
    trace |= 2;             /* skipped (unreachable) */
A:
    trace |= 4;
    goto B;
    trace |= 8;             /* skipped by goto B */
B:
    trace |= 16;

    return trace == (4 | 16) ? 0 : 1;
}
