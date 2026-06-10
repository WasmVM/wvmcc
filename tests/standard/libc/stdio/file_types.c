/* LIBC-stdio-FILE-01 — C17 7.21.1p2: <stdio.h> declares the types FILE
 * (an object type capable of recording all stream information), fpos_t
 * (a complete object type capable of recording every file position), and
 * size_t. Verify=static-assert (compiled -ffreestanding). */
#include <stdio.h>

/* All three are object types, so sizeof must be valid and nonzero. */
_Static_assert(sizeof(FILE) > 0, "FILE must be an object type");
_Static_assert(sizeof(fpos_t) > 0, "fpos_t must be a complete object type");
_Static_assert(sizeof(size_t) > 0, "size_t must be defined by <stdio.h>");

/* size_t is the unsigned result type of sizeof (7.19p2). */
_Static_assert((size_t)-1 > 0, "size_t must be an unsigned integer type");

/* The type names must be usable in declarations. */
static FILE *file_ptr;
static fpos_t pos_obj;
static size_t size_obj;
