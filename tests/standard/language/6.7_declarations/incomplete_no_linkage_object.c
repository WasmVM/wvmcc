/* LANG-6.7-05 — a no-linkage object's type must be complete by the end of
 * its declarator (6.7p7): a block-scope object of incomplete struct type
 * is rejected. */
struct incomplete;

int main(void)
{
    struct incomplete obj; /* constraint violation: incomplete type */
    return 0;
}
