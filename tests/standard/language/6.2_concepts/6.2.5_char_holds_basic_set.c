/* LANG-6.2.5-02 — 6.2.5p3: if a member of the basic execution character set is
 * stored in a char object, its value is guaranteed to be nonnegative. */

/* Each basic-execution-set member, when stored in a plain char, has a
 * nonnegative value. (CHAR_MAX >= 127 is required by <limits.h>; the sampled
 * characters below all fit and stay nonnegative.) */
_Static_assert((char)'A' >= 0, "'A' nonnegative in char");
_Static_assert((char)'z' >= 0, "'z' nonnegative in char");
_Static_assert((char)'0' >= 0, "'0' nonnegative in char");
_Static_assert((char)' ' >= 0, "space nonnegative in char");
_Static_assert((char)'\t' >= 0, "tab nonnegative in char");
_Static_assert((char)'~' >= 0, "'~' nonnegative in char");
_Static_assert((char)'\0' == 0, "null character is zero");
