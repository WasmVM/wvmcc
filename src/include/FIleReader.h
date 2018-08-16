#ifndef WVMCC_FILEREADER_DEF
#define WVMCC_FILEREADER_DEF

#include <Pass.h>
#include <Buffer.h>

Pass* new_FileReader(Buffer** input, size_t input_count, Buffer** output, size_t output_count);

#endif