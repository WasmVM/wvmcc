/* LANG-6.5.2.3-05 — Reading a union member other than the one last stored
 * reinterprets the object representation (type-punning) (ISO C17 6.5.2.3p3,
 * footnote 99). The mapping of bytes is implementation-defined; docs/spec.md
 * defines little-endian reinterpretation. */

#include <stdint.h>

union bytes {
    uint32_t  word;
    uint8_t   b[4];
};

int main(void)
{
    union bytes u;

    /* Store a known 32-bit value, then inspect its bytes. Little-endian:
     * least-significant byte first. */
    u.word = 0x11223344u;

    if (u.b[0] != 0x44) return 1;
    if (u.b[1] != 0x33) return 2;
    if (u.b[2] != 0x22) return 3;
    if (u.b[3] != 0x11) return 4;

    /* Reverse direction: assemble bytes, read back as a word. */
    u.b[0] = 0xAA;
    u.b[1] = 0xBB;
    u.b[2] = 0xCC;
    u.b[3] = 0xDD;
    if (u.word != 0xDDCCBBAAu) return 5;

    return 0;
}
