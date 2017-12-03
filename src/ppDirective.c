/**
 *    Copyright 2017 Luis Hsu
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "ppDirective.h"

int ppIndlude(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	FileInst *fileInst = *fileInstPtr;
	// Check the whole word and next 
	char *word = "include"; // "in" has been checked
	char thisChar = nextc(fileInst);
	for(int i = 2; i < 7; ++i, thisChar = nextc(fileInst)){
		if(thisChar != word[i]){
			fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, fileInst->curline);
			return -1;
		}
	}
	if(!isspace(thisChar)){
		fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, fileInst->curline);
		return -1;
	}else if(thisChar == '\n'){
		fprintf(stderr, WASMCC_ERR_EXPECT_HEADER_NAME, fileInst->curline);
		return -1;
	}
	return 0;
}
int ppIf(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppIfdef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppIfndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppElif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppElse(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppEndif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppError(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppDefine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppUndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppLine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppPragma(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}