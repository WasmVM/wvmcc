#ifndef WVMCPP_FILEINST_DEF
#define WVMCPP_FILEINST_DEF

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DELIM_CHAR '/'

typedef struct {
  FILE* fptr;
  char* fname;
  unsigned int curline;
} FileInst;

FileInst* fileInstNew(char* fname);
void fileInstFree(FileInst** finst);
int nextc(FileInst* fileInst, FILE* fout);
char* getShortName(FileInst* inst);

#endif