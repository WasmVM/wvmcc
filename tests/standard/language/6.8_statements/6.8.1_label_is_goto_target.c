/* LANG-6.8.1-01 — Any statement may be preceded by an identifier label, which
 * declares the identifier as a label name usable as the target of a `goto`
 * (ISO C17 6.8.1p1, 6.8.1p5). */

int main(void)
{
    int trace = 0;

    goto forward;
    return 1; /* skipped */

forward:
    trace += 1;
    if (trace == 1)
        goto backward_target;
    return 2;

done:
    if (trace != 3) return 3;
    return 0;

backward_target:
    trace += 2; /* trace becomes 3 */
    goto done;  /* labels have function scope: backward jump works too */
}
