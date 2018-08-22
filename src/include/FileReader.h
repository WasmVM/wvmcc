#ifndef WVMCC_FILEREADER_DEF
#define WVMCC_FILEREADER_DEF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <adt/Pass.h>
#include <adt/Buffer.h>

typedef Pass FileReader;
Pass* new_FileReader(Buffer** input, size_t input_count, Buffer** output, size_t output_count);

#endif
