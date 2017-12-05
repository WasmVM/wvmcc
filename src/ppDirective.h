#ifndef WASMCC_PPDIRECTIVE_DEF
#define WASMCC_PPDIRECTIVE_DEF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "fileInst.h"
#include "stack.h"
#include "errorMsg.h"
#include "map.h"

extern char *defInclPath;

int ppIndlude(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppIf(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppIfdef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppIfndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppElif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppElse(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppEndif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppError(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppDefine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppUndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppLine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppPragma(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
#endif // !WASMCC_PPDIRECTIVE_DEF