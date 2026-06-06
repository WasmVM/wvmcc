/* LANG-6.2.1-01 — 6.2.1p3: a label name has function scope; a goto can
 * reach a label declared anywhere in the same function, including later. */
int main(void)
{
    int hits = 0;

    goto forward;          /* jump to a label that appears later */
back:
    if (hits == 2) return 0;   /* success: reached after forward + back */
    return 1;                  /* should not fall through here */

forward:
    hits = 1;
    goto deeper;

deeper:
    hits = 2;
    goto back;             /* jump backward to an earlier label */
}
