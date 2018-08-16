#ifndef WVMCC_FILEREADER_DEF
#define WVMCC_FILEREADER_DEF

#include <stdlib.h>
#include <Pass.h>
#include <adt/Buffer.h>

Pass* new_FileReader(Buffer** input, size_t input_count, Buffer** output, size_t output_count);

#endif