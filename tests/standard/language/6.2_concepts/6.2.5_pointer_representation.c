/* LANG-6.2.5-15 — 6.2.5p28: a pointer to void shall have the same representation
 * and alignment requirements as a pointer to a character type. Pointers to
 * compatible types shall have the same representation and alignment. */

/* void* and char* share size and alignment. */
_Static_assert(sizeof(void *) == sizeof(char *), "void* size == char* size");
_Static_assert(_Alignof(void *) == _Alignof(char *), "void* align == char* align");

/* The three character-pointer types share representation/alignment with void*. */
_Static_assert(sizeof(void *) == sizeof(signed char *), "void* size == signed char*");
_Static_assert(sizeof(void *) == sizeof(unsigned char *), "void* size == unsigned char*");

/* Pointers to compatible types share representation; const-qualifying the
 * pointee does not change the pointer's size or alignment. */
_Static_assert(sizeof(int *) == sizeof(const int *), "int* and const int* same size");
_Static_assert(_Alignof(int *) == _Alignof(const int *), "int* and const int* same align");
