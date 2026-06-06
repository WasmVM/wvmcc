/* LANG-6.2.4-02 — 6.2.4p5,p6: an object with automatic storage duration is
 * created on each entry to its block; a fresh instance exists per recursion. */

static int depth_seen[4];

static int recurse(int level) {
    int local = level;          /* new instance on every entry */
    depth_seen[level] = local;
    if (level + 1 < 4) {
        if (recurse(level + 1) != 0) return 1;
    }
    /* The local instance for this frame must be undisturbed by deeper calls. */
    if (local != level) return 2;
    if (depth_seen[level] != level) return 3;
    return 0;
}

int main(void) {
    if (recurse(0) != 0) return 1;
    if (depth_seen[0] != 0) return 2;
    if (depth_seen[1] != 1) return 3;
    if (depth_seen[2] != 2) return 4;
    if (depth_seen[3] != 3) return 5;
    return 0;
}
