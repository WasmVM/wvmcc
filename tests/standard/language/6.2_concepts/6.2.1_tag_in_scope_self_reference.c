/* LANG-6.2.1-05 — 6.2.1p7: a struct/union/enum tag is in scope immediately
 * after it appears, so a member may be a pointer to the same struct type. */
struct node {
    int value;
    struct node *next;   /* tag 'node' already in scope here */
};

int main(void)
{
    struct node b = { 2, 0 };
    struct node a = { 1, &b };
    if (a.value != 1) return 1;
    if (a.next->value != 2) return 2;
    if (a.next->next != 0) return 3;
    return 0;
}
