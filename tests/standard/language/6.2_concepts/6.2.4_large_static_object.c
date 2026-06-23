/* LANG-6.2.4-04 — 6.2.4p2: an object with static storage duration exists for
 * the entire program and (6.7.9p10) is zero-initialized when no initializer is
 * given. A large uninitialized (.bss) object must be fully addressable: it
 * produces no data segment, so the linker must still size mem[0] to cover its
 * extent (regression for the .bss-extent sizing fix; a >64 KiB object spans
 * past the first page). */

static unsigned char big[65535];   /* .bss: no initializer, no data segment */

int main(void) {
    /* Zero-initialized per 6.7.9p10. */
    for (unsigned long i = 0; i < sizeof big; i++)
        if (big[i] != 0) return 1;
    /* Both ends — including the byte beyond the first 64 KiB page — writable. */
    big[0] = 1;
    big[sizeof big - 1] = 2;
    if (big[0] != 1) return 2;
    if (big[sizeof big - 1] != 2) return 3;
    return 0;
}
