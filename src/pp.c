//    Copyright 2017 Luis Hsu
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errorMsg.h"
#include "fileInst.h"
#include "map.h"
#include "ppDirective.h"
#include "stack.h"

char* defInclPath = NULL;

int parsePP(FileInst** fileInstPtr,
            Stack* fileStack,
            FILE* fout,
            Map* macroMap,
            int* skipPtr) {
  char thisChar;
  FileInst* fileInst = *fileInstPtr;
  // Trim leading space
  while (isspace(thisChar = nextc(fileInst, fout)) && thisChar != '\n')
    ;
  ungetc(thisChar, fileInst->fptr);
  // Read char
  while ((thisChar = nextc(fileInst, fout)) != '\n' &&
         (!*skipPtr || thisChar == 'e')) {
    switch (thisChar) {
      case 'i':
        thisChar = nextc(fileInst, fout);
        switch (thisChar) {
          case 'f':
            thisChar = nextc(fileInst, fout);
            switch (thisChar) {
              case 'd':  // ifdef
                return ppIfdef(fileInstPtr, fileStack, fout, macroMap, skipPtr);
                break;
              case 'n':  // ifndef
                return ppIfndef(fileInstPtr, fileStack, fout, macroMap,
                                skipPtr);
                break;
              default:  // if
                return ppIf(fileInstPtr, fileStack, fout, macroMap, skipPtr);
                break;
            }
            break;
          case 'n':  // include
            return ppIndlude(fileInstPtr, fileStack, fout, macroMap);
            break;
          default:
            fprintf(stderr, WVMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst),
                    fileInst->curline);
            return -1;
            break;
        }
        break;
      case 'e':
        thisChar = nextc(fileInst, fout);
        switch (thisChar) {
          case 'l':
            thisChar = nextc(fileInst, fout);
            switch (thisChar) {
              case 'i':  // elif
                return ppElif(fileInstPtr, fileStack, fout, macroMap, skipPtr);
                break;
              case 's':  // else
                return ppElse(fileInstPtr, fileStack, fout, skipPtr);
                break;
              default:
                fprintf(stderr, WVMCC_ERR_NON_PP_DIRECTIVE,
                        getShortName(fileInst), fileInst->curline);
                return -1;
                break;
            }
            break;
          case 'n':  // endif
            return ppEndif(fileInstPtr, fileStack, fout, skipPtr);
            break;
          case 'r':  // error
            if (!*skipPtr) {
              return ppError(fileInstPtr, fileStack, fout, macroMap);
            } else {
              while ((thisChar = nextc(fileInst, fout)) != '\n')
                ;
            }
            break;
          default:
            fprintf(stderr, WVMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst),
                    fileInst->curline);
            return -1;
            break;
        }
        break;
      case 'p':  // pragma
        return ppPragma(fileInstPtr, fileStack, fout);
        break;
      case 'd':  // define
        return ppDefine(fileInstPtr, fileStack, fout, macroMap);
        break;
      case 'u':  // undef
        return ppUndef(fileInstPtr, fileStack, fout, macroMap);
        break;
      case 'l':  // line
        return ppLine(fileInstPtr, fileStack, fout, macroMap);
        break;
      default:
        fprintf(stderr, WVMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst),
                fileInst->curline);
        return -1;
        break;
    }
  }
  return 0;
}

int scan(Stack* fileStack, FILE* fout, Map* macroMap) {
  FileInst* fileInst = NULL;
  int skip = 0;
  // Process file
  while (!stackPop(fileStack, (void**)&fileInst)) {
    int thisChar;
    while ((thisChar = nextc(fileInst, fout)) != EOF) {
      // New Line
      if (thisChar == '\n') {
        ++fileInst->curline;
      }
      // Digraph
      if (thisChar == '%') {
        int nextChar = nextc(fileInst, fout);
        if (nextChar == ':') {
          thisChar = '#';
        } else {
          ungetc(nextChar, fileInst->fptr);
        }
      }
      // Trigraph
      if (thisChar == '?') {
        int nextChar = nextc(fileInst, fout);
        int thirdChar = nextc(fileInst, fout);
        if (nextChar == '?' && thirdChar == '=') {
          thisChar = '#';
        } else {
          ungetc(nextChar, fileInst->fptr);
          ungetc(thirdChar, fileInst->fptr);
        }
      }
      if (thisChar == '#') {
        // Proprocessor line
        if (parsePP(&fileInst, fileStack, fout, macroMap, &skip)) {
          fileInstFree(&fileInst);
          return -1;
        } else {
          fputc('\n', fout);
        }
      } else if (!skip) {
        // Normal
        char* lineStr = calloc(4096, sizeof(char));
        memset(lineStr, 0, 4096);
        for (int i = 0; thisChar != '\n' && thisChar != EOF;
             ++i, thisChar = nextc(fileInst, fout)) {
          lineStr[i] = thisChar;
        }
        lineStr = realloc(lineStr, strlen(lineStr) + 1);
        char* expanded = expandMacro(lineStr, macroMap, fileInst, fout);
        if (expanded) {
          fwrite(expanded, 1, strlen(expanded), fout);
        } else {
          fileInstFree(&fileInst);
          return -1;
        }
        if (thisChar == '\n') {
          fputc(thisChar, fout);
        }
        free(expanded);
        free(lineStr);
      }
    }
    // End processing this file
    FileInst* topFile = NULL;
    if (!stackTop(fileStack, (void**)&topFile)) {
      fprintf(fout, "\n# %u \"%s\" 2\n", topFile->curline,
              topFile->fname);  // 2: return to file
    }
    fileInstFree(&fileInst);
  }
  return 0;
}

int main(int argc, char* argv[]) {
  // Check argc
  if (argc < 3) {
    if (argc < 2) {
      fprintf(stderr, "[PP] No input file.\n");
    } else {
      fprintf(stderr, "[PP] No output file.\n");
    }
    return -1;
  }
  // Get wasmcpp path
  int cppPathLen = strrchr(argv[0], DELIM_CHAR) - argv[0];
  defInclPath = (char*)calloc(cppPathLen + 9, sizeof(char));
  memcpy(defInclPath, argv[0], cppPathLen);
  defInclPath[cppPathLen] = DELIM_CHAR;
  strcat(defInclPath, "include");
  // Alloc main file instance
  char* mainPath =
      (char*)calloc(cppPathLen + strlen(argv[1]) + 2, sizeof(char));
  memcpy(mainPath, argv[0], cppPathLen);
  mainPath[cppPathLen] = DELIM_CHAR;
  strcat(mainPath, argv[1]);
  FileInst* mainFile = fileInstNew(mainPath);
  free(mainPath);
  if (mainFile == NULL) {
    return -2;
  }
  // Open output file
  FILE* fout = fopen(argv[2], "wb");
  if (errno) {
    perror(argv[2]);
    return -3;
  }
  fprintf(fout, "# %d \"%s\"\n", mainFile->curline, mainFile->fname);
  // File stack
  Stack* fileStack = stackNew();
  stackPush(fileStack, mainFile);
  // Macro Map
  Map* macroMap = mapNew(compMacro, freeMacroName, freeMacro);
  // Process
  int parseErr = scan(fileStack, fout, macroMap);
  // Clean
  // Macro map
  mapFree(&macroMap);
  // Cpp path
  free(defInclPath);
  // Close output
  fclose(fout);
  // Free stack
  stackFree(&fileStack);
  if (parseErr) {
    remove(argv[2]);
    return -1;
  }
  return 0;
}