/* LANG-5.1.2.1-02 — 5.1.2.1p2: the effect of program termination in a
 * freestanding environment is implementation-defined. wvmcc (docs/spec.md):
 * control returns to WasmVM and `sys_proc.exit` sets the process exit code.
 * Successful termination here must yield exit status 0 as observed by the
 * host harness. */
int main(void)
{
    int status = 7;
    status -= 7;                /* computed zero: termination status is 0 */
    return status;
}
