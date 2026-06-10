/* LANG-6.6-06 — Address constants (ISO C17 6.6p9): a null pointer, a
 * pointer to an lvalue designating an object of static storage duration
 * (`&` or array decay), or a pointer to a function designator, optionally
 * formed with [], ., ->, & and * without accessing the object's value.
 * These may initialize objects with static storage duration. */

static int obj = 42;
static int arr[4] = {10, 20, 30, 40};
struct pair { int a, b; };
static struct pair pr = {1, 2};

static int func(void) { return 7; }

/* `&` of a static-duration object. */
static int *p_obj = &obj;

/* Array decay and element addresses. */
static int *p_arr = arr;            /* array-to-pointer decay */
static int *p_elem = &arr[2];       /* &*(arr + 2) */
static int *p_off = arr + 1;        /* address constant plus integer constant */

/* Member address via `.` without evaluating the object. */
static int *p_memb = &pr.b;

/* Function designators: implicit decay and explicit `&`. */
static int (*fp1)(void) = func;
static int (*fp2)(void) = &func;

/* Null pointer constant. */
static int *p_null = 0;

int main(void)
{
    if (p_obj != &obj) return 1;
    if (*p_obj != 42) return 2;
    if (p_arr != &arr[0]) return 3;
    if (*p_elem != 30) return 4;
    if (p_off != &arr[1]) return 5;
    if (*p_off != 20) return 6;
    if (*p_memb != 2) return 7;
    if (fp1 != fp2) return 8;
    if (fp1() != 7) return 9;
    if (p_null != (int *)0) return 10;
    return 0;
}
