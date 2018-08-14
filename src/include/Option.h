#ifndef WVMCC_OPTION_DEF
#define WVMCC_OPTION_DEF

#include <string.h>
#include <stdlib.h>

typedef struct _Option{
    char* output_file;
    void (*free)(struct _Option* this);
} Option;

Option* new_Option(int argc, const char* argv[]);

#endif