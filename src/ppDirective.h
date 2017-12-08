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
#include "list.h"

extern char *defInclPath;

typedef struct {
	char *str;
	int hasVA;
	List *params;
} MacroInst;

int compMacro(void *aPtr, void* bPtr);
void freeMacroName(void *ptr);
void freeMacro(void *ptr);

char *expandMacro(char *line, Map* macroMap);

int ppIndlude(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppIf(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppIfdef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppIfndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppElif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppElse(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppEndif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
int ppError(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppDefine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppUndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppLine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap);
int ppPragma(FileInst **fileInstPtr, Stack *fileStack, FILE *fout);
#endif // !WASMCC_PPDIRECTIVE_DEF