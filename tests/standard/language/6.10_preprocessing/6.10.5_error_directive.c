/* LANG-6.10.5-01 — a #error directive produces a diagnostic and fails
 * translation (6.10.5). Verify=compile-fail: wvmcc must reject this TU. */
#error "LANG-6.10.5-01: this #error must fail translation"
int main(void) { return 0; }
