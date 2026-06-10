/* LANG-6.8.1-03 — Label names shall be unique within a function
 * (ISO C17 6.8.1p3). Defining the same label twice in one function is a
 * constraint violation a conforming compiler must reject. */

int main(void)
{
    int n = 0;
again:
    ++n;
    if (n < 2)
        goto again;
again: /* error: duplicate label `again` in the same function */
    return 0;
}
