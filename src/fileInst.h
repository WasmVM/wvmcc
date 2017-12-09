#ifndef WASMCPP_FILEINST_DEF
#define WASMCPP_FILEINST_DEF

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define DELIM_CHAR '/'

typedef struct {
	FILE *fptr;
	char *fname;
	unsigned int curline;
} FileInst;

FileInst *fileInstNew(char *fname);
void fileInstFree(FileInst **finst);
int nextc(FileInst *fileInst, FILE *fout);
char *getShortName(FileInst *inst);

#endif