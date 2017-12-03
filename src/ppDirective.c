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
	char *word = "clude"; // "in" has been checked
	char thisChar = nextc(fileInst);
	for(int i = 0; i < 5; ++i, thisChar = nextc(fileInst)){
		if(thisChar != word[i]){
			fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
			return -1;
		}
	}
	if(!isspace(thisChar)){
		fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
		return -1;
	}else if(thisChar == '\n'){
		fprintf(stderr, WASMCC_ERR_EXPECT_HEADER_NAME, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Read header name
	thisChar = nextc(fileInst);
	long int curFilePos = ftell(fileInst->fptr);
	int headerLength = 0; 
	if(thisChar == '<'){ // h-char-sequence
		for(thisChar = nextc(fileInst); thisChar != '>'; thisChar = nextc(fileInst), ++headerLength){
			if(thisChar == '\n'){
				fprintf(stderr, WASMCC_ERR_H_CHAR_HEADER_NOEND, getShortName(fileInst), fileInst->curline);
				return -1;
			}
		}
	}else if(thisChar == '\"'){ // q-char-sequence
		for(thisChar = nextc(fileInst); thisChar != '\"'; thisChar = nextc(fileInst), ++headerLength){
			if(thisChar == '\n'){
				fprintf(stderr, WASMCC_ERR_Q_CHAR_HEADER_NOEND, getShortName(fileInst), fileInst->curline);
				return -1;
			}
		}
	}else{
		// TODO: replace normal text by macro
		fprintf(stderr, WASMCC_ERR_EXPECT_HEADER_NAME, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	fseek(fileInst->fptr, curFilePos, SEEK_SET);
	char *headerName = (char *)calloc(headerLength + 1, sizeof(char));
	for(int i = 0; i < headerLength; ++i){
		headerName[i] = nextc(fileInst);
	}
	thisChar = nextc(fileInst);
	// Get header file path
	char *headerPath = NULL;
	if(thisChar == '>'){
		headerPath = (char *)calloc(strlen(defInclPath) + headerLength + 1, sizeof(char));
		strcpy(headerPath, defInclPath);
		headerPath[strlen(defInclPath)] = DELIM_CHAR;
		strcat(headerPath, headerName);
	}else{
		char *rchr = strrchr(fileInst->fname, DELIM_CHAR);
		if(rchr){
			int rchrLen = rchr - fileInst->fname;
			headerPath = (char *)calloc(rchrLen + headerLength + 1, sizeof(char));
			memcpy(headerPath, fileInst->fname, rchrLen);
			headerPath[rchrLen] = DELIM_CHAR;
			strcat(headerPath, headerName);
		}else{
			headerPath = (char *)calloc(headerLength, sizeof(char));
			strcpy(headerPath, headerName);
		}
	}
	// Include
	FileInst *inclFile = fileInstNew(headerPath);
	if(inclFile == NULL){
		return -1;
	}
	stackPush(fileStack, fileInst);
	*fileInstPtr = inclFile;
	fprintf(fout, "# 1 %s 1", headerPath);
	// Clean
	free(headerName);
	free(headerPath);
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