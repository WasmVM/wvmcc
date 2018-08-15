#ifndef WVMCC_OPTION_DEF
#define WVMCC_OPTION_DEF

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WVMCC_VERSION
#define WVMCC_VERSION "dev"
#endif

typedef struct _Option{
    const char* output_file;
    const char** input_files;
    size_t input_count;
    void (*free)(struct _Option *thisOption);
} Option;

Option* new_Option(int argc, const char* argv[]);

#endif