/* tests/standard/libc/inttypes/imaxdiv_t.c — LIBC-inttypes-imaxdiv_t-01.
 * ISO C17 §7.8p1: <inttypes.h> declares imaxdiv_t, a structure type that is
 * the type of the value returned by imaxdiv, with members quot and rem of
 * type intmax_t. Verify=static-assert. */
#include <inttypes.h>

/* imaxdiv_t must be a complete object type with intmax_t members quot/rem. */
_Static_assert(sizeof(imaxdiv_t) >= 2 * sizeof(intmax_t), "imaxdiv_t holds quot and rem");
_Static_assert(sizeof(((imaxdiv_t *)0)->quot) == sizeof(intmax_t), "quot is intmax_t-sized");
_Static_assert(sizeof(((imaxdiv_t *)0)->rem) == sizeof(intmax_t), "rem is intmax_t-sized");

int main(void) { return 0; }
