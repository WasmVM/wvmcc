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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "stack.h"
#include "fileInst.h"
#include "ppDirective.h"
#include "ErrorMsg.h"

int parsePP(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	char thisChar;
	FileInst *fileInst = *fileInstPtr;
	// Trim leading space
	while(isspace(thisChar = nextc(fileInst)) && thisChar != '\n');
	ungetc(thisChar, fileInst->fptr);
	// Read char
	while((thisChar = nextc(fileInst)) != '\n'){
		switch(thisChar){
			case 'i': 
				thisChar = nextc(fileInst);
				switch(thisChar){
					case 'f': 
						thisChar = nextc(fileInst);
						switch(thisChar){
							case 'd': // ifdef
								return ppIfdef(fileInstPtr, fileStack, fout);
							break;
							case 'n': // ifndef
								return ppIfndef(fileInstPtr, fileStack, fout);
							break;
							default: // if
								return ppIf(fileInstPtr, fileStack, fout);
							break;
						}
					break;
					case 'n': // include
						return ppIndlude(fileInstPtr, fileStack, fout);
					break;
					default: 
						fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, fileInst->curline);
						return -1;
					break;
				}
			break;
			case 'e': 
				switch(thisChar){
					case 'l': 
						thisChar = nextc(fileInst);
						switch(thisChar){
							case 'i': // elif
								return ppElif(fileInstPtr, fileStack, fout);
							break;
							case 's': // else
								return ppElse(fileInstPtr, fileStack, fout);
							break;
							default:
								fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, fileInst->curline);
								return -1;
							break;
						}
					break;
					case 'n': // endif
						return ppEndif(fileInstPtr, fileStack, fout);
					break;
					case 'r': // error
						return ppError(fileInstPtr, fileStack, fout);
					break;
					default: 
						fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, fileInst->curline);
						return -1;
					break;
				}
			break;
			case 'p': // pragma
				return ppPragma(fileInstPtr, fileStack, fout);
			break;
			case 'd': // define
				return ppDefine(fileInstPtr, fileStack, fout);
			break;
			case 'u': // undef
				return ppUndef(fileInstPtr, fileStack, fout);
			break;
			case 'l': // line
				return ppLine(fileInstPtr, fileStack, fout);
			break;
			default: 
				fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, fileInst->curline);
				return -1;
			break;
		}
	}
	return 0;
}

int scan(Stack *fileStack, FILE *fout){
	FileInst *fileInst = NULL;
	// Process file
	while(stackPop(fileStack, (void **)&fileInst)){
		int thisChar;
		while((thisChar = nextc(fileInst)) != EOF){
			// New Line
			if(thisChar == '\n'){
				++fileInst->curline;
			}
			// Digraph
			if(thisChar == '%'){
				int nextChar = nextc(fileInst);
				if(nextChar == ':'){
					thisChar = '#';
				}else{
					ungetc(nextChar, fileInst->fptr);
				}
			}
			// Trigraph
			if(thisChar == '?'){
				int nextChar = nextc(fileInst);
				int thirdChar = nextc(fileInst);
				if(nextChar == '?' && thirdChar == '='){
					thisChar = '#';
				}else{
					ungetc(nextChar, fileInst->fptr);
					ungetc(thirdChar, fileInst->fptr);
				}
			}
			if((fileInst->lastChar == '\n') && (thisChar == '#')){
				// Proprocessor line
				if(parsePP(&fileInst, fileStack, fout)){
					fileInstFree(&fileInst);
					return -1;
				}else{
					fputc('\n', fout);
					fileInst->lastChar = '\n';
				}
			}else{
				// Normal
				fputc(thisChar, fout);
				fileInst->lastChar = thisChar;
			}
		}
		// End processing this file
		FileInst *topFile = NULL;
		if(stackTop(fileStack, (void **)&topFile)){		
			fprintf(fout, "\n# %u %s 2\n", topFile->curline, topFile->fname);
		}
		fileInstFree(&fileInst);
	}
	return 0;
}

int main(int argc, char *argv[]){
// Prepare
	// Check argc
	if(argc < 3){
		if(argc < 2){
			fprintf(stderr, "[PP] No input file.\n");
		}else{
			fprintf(stderr, "[PP] No output file.\n");
		}
		return -1;
	}
	// Alloc main file instance
	FileInst *mainFile = fileInstNew(argv[1]);
	if(mainFile == NULL){
		return -2;
	}
	// Open output file
	FILE *fout = fopen(argv[2], "wb");
	if(errno){
		perror(argv[2]);
		return -3;
	}
	// File stack
	Stack *fileStack = stackNew();
	stackPush(fileStack, mainFile);
// Process
	int parseErr = scan(fileStack, fout);
// Clean
	// Close output
	fclose(fout);
	// Free stack
	stackFree(&fileStack);
	if(parseErr){
		remove(argv[2]);
		return -1;
	}
	return 0;
}