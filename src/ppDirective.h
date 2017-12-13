#ifndef WASMCC_PPDIRECTIVE_DEF
#define WASMCC_PPDIRECTIVE_DEF

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "errorMsg.h"
#include "fileInst.h"
#include "list.h"
#include "map.h"
#include "stack.h"

extern char* defInclPath;

typedef struct {
  char* str;
  int hasVA;
  int enable;
  List* params;
} MacroInst;

int compMacro(void* aPtr, void* bPtr);
void freeMacroName(void* ptr);
void freeMacro(void* ptr);

char* expandMacro(char* line, Map* macroMap, FileInst* fileInst, FILE* fout);

int ppIndlude(FileInst** fileInstPtr,
              Stack* fileStack,
              FILE* fout,
              Map* macroMap);
int ppIf(FileInst** fileInstPtr,
         Stack* fileStack,
         FILE* fout,
         Map* macroMap,
         int* skipPtr);
int ppIfdef(FileInst** fileInstPtr,
            Stack* fileStack,
            FILE* fout,
            Map* macroMap,
            int* skipPtr);
int ppIfndef(FileInst** fileInstPtr,
             Stack* fileStack,
             FILE* fout,
             Map* macroMap,
             int* skipPtr);
int ppElif(FileInst** fileInstPtr,
           Stack* fileStack,
           FILE* fout,
           Map* macroMap,
           int* skipPtr);
int ppElse(FileInst** fileInstPtr, Stack* fileStack, FILE* fout, int* skipPtr);
int ppEndif(FileInst** fileInstPtr, Stack* fileStack, FILE* fout, int* skipPtr);
int ppError(FileInst** fileInstPtr,
            Stack* fileStack,
            FILE* fout,
            Map* macroMap);
int ppDefine(FileInst** fileInstPtr,
             Stack* fileStack,
             FILE* fout,
             Map* macroMap);
int ppUndef(FileInst** fileInstPtr,
            Stack* fileStack,
            FILE* fout,
            Map* macroMap);
int ppLine(FileInst** fileInstPtr, Stack* fileStack, FILE* fout, Map* macroMap);
int ppPragma(FileInst** fileInstPtr, Stack* fileStack, FILE* fout);
#endif  // !WASMCC_PPDIRECTIVE_DEF