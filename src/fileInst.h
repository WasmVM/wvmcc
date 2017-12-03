#ifndef WASMCPP_FILEINST_DEF
#define WASMCPP_FILEINST_DEF

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	FILE *fptr;
	char *fname;
	int lastChar;
	unsigned int curline;
} FileInst;

FileInst *fileInstNew(char *fname);
void fileInstFree(FileInst **finst);

#endif