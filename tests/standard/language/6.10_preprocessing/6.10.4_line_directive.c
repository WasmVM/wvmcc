/* LANG-6.10.4-01 — #line sets the line number that __LINE__ reports for the
 * following source line (6.10.4). Verify=exit: __LINE__ reflects the override. */
int main(void) {
#line 100
    return (__LINE__ == 100) ? 0 : 1;
}
