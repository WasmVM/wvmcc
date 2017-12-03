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

void parsePP(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	char thisChar;
	FileInst *fileInst = *fileInstPtr;
	// Trim leading space
	while(isspace(thisChar = fgetc(fileInst->fptr)) && thisChar != '\n');
	ungetc(thisChar, fileInst->fptr);
	// Read char
	while((thisChar = fgetc(fileInst->fptr)) != '\n'){
		fputc(thisChar, stderr);
	}
}

void scan(Stack *fileStack, FILE *fout){
	FileInst *fileInst = NULL;
	while(stackPop(fileStack, (void **)&fileInst)){
		int thisChar;
		while((thisChar = fgetc(fileInst->fptr)) != EOF){
			// Digraph
			if(thisChar == '%'){
				int nextChar = fgetc(fileInst->fptr);
				if(nextChar == ':'){
					thisChar = '#';
				}else{
					ungetc(nextChar, fileInst->fptr);
				}
			}
			// Trigraph
			if(thisChar == '?'){
				int nextChar = fgetc(fileInst->fptr);
				int thirdChar = fgetc(fileInst->fptr);
				if(nextChar == '?' && thirdChar == '='){
					thisChar = '#';
				}else{
					ungetc(nextChar, fileInst->fptr);
					ungetc(thirdChar, fileInst->fptr);
				}
			}
			if((fileInst->lastChar == '\n') && (thisChar == '#')){
				// Proprocessor line
				parsePP(&fileInst, fileStack, fout);
				fputc('\n', fout);
				fileInst->lastChar = '\n';
			}else{
				// Normal
				fputc(thisChar, fout);
				fileInst->lastChar = thisChar;
			}
		}
		FileInst *topFile = NULL;
		if(stackTop(fileStack, (void **)&topFile)){		
			fprintf(fout, "\n# %u %s 2\n", topFile->curline, topFile->fname);
		}
		fileInstFree(&fileInst);
	}
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
	scan(fileStack, fout);
// Clean
	// Close output
	fclose(fout);
	// Free stack
	stackFree(&fileStack);
	return 0;
}