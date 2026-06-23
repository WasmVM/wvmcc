/* LANG-6.2.4-05 — 6.2.4p5,p6: an object with automatic storage duration is
 * created on entry to its block. A large automatic object must be fully
 * addressable within its frame: the shadow stack must be sized to the largest
 * single frame, not a fixed 64 KiB page (regression for the shadow-stack
 * sizing fix; this frame exceeds one page). */

int main(void) {
    volatile char buf[70000];      /* single frame > 64 KiB */
    buf[0] = 1;
    buf[sizeof buf - 1] = 2;       /* byte past the first page */
    if (buf[0] != 1) return 1;
    if (buf[sizeof buf - 1] != 2) return 2;
    return 0;
}
