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

int compMacro(void *aPtr, void* bPtr){
	return strcmp((char *)aPtr, (char *)bPtr);
}

void freeMacroName(void *ptr){
	free((char *)ptr);
}

void freeMacro(void *ptr){
	MacroInst *inst = (MacroInst *)ptr;
	free(inst->str);
	listFree(&inst->params);
	free(inst);
}

char *expandMacro(char *line, Map* macroMap){
	char *ret = (char *)calloc(4096, sizeof(char));
	// TODO: Skip string literal
	// TODO: Skip character-constant
	// TODO: Skip integer-constant
	// TODO: Skip float-constant
	return realloc(ret, strlen(ret) + 1);
}

static char *getIdent(char *thisCharPtr, FileInst *fileInst){
	char thisChar = *thisCharPtr;
	char *ident = (char *)calloc(65, sizeof(char));
	if(isalpha(thisChar) || thisChar == '_'){
		memset(ident, '\0', 64 * sizeof(char));
		ident[0] = thisChar;
		for(int i = 1; i <= 64; ++i){
			if(i == 64){
				*thisCharPtr = thisChar;
				fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
				return NULL;
			}
			thisChar = nextc(fileInst);
			if(!isalnum(thisChar) && thisChar != '_'){
				break;
			}
			ident[i] = thisChar;
		}
	}else{
		*thisCharPtr = thisChar;
		fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
		return NULL;
	}
	ident = (char *)realloc(ident, strlen(ident) + 1);
	*thisCharPtr = thisChar;
	return ident;
}

int ppIndlude(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
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
	// TODO: macro
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
int ppIf(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	return 0;
}
int ppIfdef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	return 0;
}
int ppIfndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	return 0;
}
int ppElif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	return 0;
}
int ppElse(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppEndif(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}
int ppError(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	return 0;
}
int ppDefine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	FileInst *fileInst = *fileInstPtr;
	// Check the whole word and next 
	char *word = "efine"; // "d" has been checked
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
		fprintf(stderr, WASMCC_ERR_EXPECT_MACRO_NAME, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Read macro name
	thisChar = nextc(fileInst);
	char *macroName = getIdent(&thisChar, fileInst);
	if(!macroName){
		return -1;
	}
	if((!isspace(thisChar) && thisChar != '(')){
		fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Alloc macro instance
	MacroInst *newMacro = (MacroInst *)malloc(sizeof(MacroInst));
	newMacro->hasVA = 0;
	newMacro->params = NULL;
	// Read parameter
	if(thisChar == '('){
		newMacro->params = listNew();
		thisChar = nextc(fileInst);
		while(thisChar != ')'){
			// Trim leading space
			while(isspace(thisChar) && thisChar != '\n'){
				thisChar = nextc(fileInst);
			}
			if(thisChar == '\n'){
				fprintf(stderr, WASMCC_ERR_INVALID_MACRO_PARAM, getShortName(fileInst), fileInst->curline);
				free(newMacro);
				listFree(&newMacro->params);
				return -1;
			}else if(thisChar == ')'){
				break;
			}else if(thisChar == '.'){
				// Variation argument
				if((nextc(fileInst)!= '.') || (nextc(fileInst) != '.')){
					fprintf(stderr, WASMCC_ERR_EXPECT_VA, getShortName(fileInst), fileInst->curline);
					free(newMacro);
					listFree(&newMacro->params);
					return -1;
				}
				// Trim trailing space
				while(isspace(thisChar = nextc(fileInst)) && thisChar != '\n');
				if(thisChar != ')'){
					fprintf(stderr, WASMCC_ERR_VA_NOT_END, getShortName(fileInst), fileInst->curline);
					free(newMacro);
					listFree(&newMacro->params);
					return -1;
				}
				newMacro->hasVA = 1;
			}else{
				// Get identifier
				char *ident = getIdent(&thisChar, fileInst);
				if(!ident){
					fprintf(stderr, WASMCC_ERR_INVALID_MACRO_PARAM, getShortName(fileInst), fileInst->curline);
					free(newMacro);
					listFree(&newMacro->params);
					return -1;
				}
				listAdd(newMacro->params, ident);
				// Trim trailing space
				while(isspace(thisChar) && thisChar != '\n'){
					thisChar = nextc(fileInst);
				}
				switch(thisChar){
					case '\n':
						fprintf(stderr, WASMCC_ERR_INVALID_MACRO_PARAM, getShortName(fileInst), fileInst->curline);
						free(newMacro);
						listFree(&newMacro->params);
					return -1;
					case ',':
						thisChar = nextc(fileInst);
					case ')':
					break;
					default:
						fprintf(stderr, WASMCC_ERR_INVALID_MACRO_PARAM, getShortName(fileInst), fileInst->curline);
						free(newMacro);
						listFree(&newMacro->params);
					return -1;
				}
			}
		}
	}
	// Read macro string
	char *macroStr = (char *)calloc(4096, sizeof(char));
	for(int i = -1; thisChar != '\n'; ++i){
		if(i >= 0){
			macroStr[i] = thisChar;
		}
		thisChar = nextc(fileInst);
	}
	newMacro->str = realloc(macroStr, strlen(macroStr) + 1);
	mapInsert(macroMap, macroName, newMacro);
	return 0;
}
int ppUndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	return 0;
}
int ppLine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	return 0;
}
int ppPragma(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	return 0;
}