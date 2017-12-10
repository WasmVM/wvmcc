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
	int ret = strcmp((char *)aPtr, (char *)bPtr);
	return (ret > 0) - (ret < 0);
}

void freeMacroName(void *ptr){
	free((char *)ptr);
}

void freeMacro(void *ptr){
	MacroInst *inst = (MacroInst *)ptr;
	if(inst){
		free(inst->str);
		if(inst->params){
			ListNode *cur = inst->params->head;
			while(cur != NULL){
				ListNode *node = cur;
				cur = cur->next;
				free(node->data);
			}
		}
		listFree(&inst->params);
		free(inst);
	}
}

char *expandMacro(char *line, Map* macroMap, FileInst *fileInst, FILE *fout){
	char *ret = NULL;
	char *oriLine = line;
	int lineRescan = 0;
	do{
		ret = (char *)calloc(4096, sizeof(char));
		lineRescan = 0;
		List *disabledMacro = listNew();
		// Expand
		for(int charIndex = 0, retIndex = 0, lineSize = strlen(line); charIndex < lineSize; ++charIndex, ++retIndex){
			while(isspace(line[charIndex])){
				ret[retIndex++] = line[charIndex++];
			}
			if(line[charIndex] == '\"' || line[charIndex] == '\''){ // Skip string literal
				char quot = line[charIndex];
				do{
					ret[retIndex++] = line[charIndex++];
				}while(charIndex < lineSize && (line[charIndex] != quot || line[charIndex - 1] == '\\') && line[charIndex] != '\n');
			}else if(isdigit(line[charIndex])){ // Skip constant
				// digit sequence
				if((charIndex < lineSize - 1) && (line[charIndex] == '0') && (line[charIndex + 1] == 'x' || line[charIndex + 1] == 'X')){
					ret[retIndex] = line[charIndex];
					ret[retIndex + 1] = line[charIndex + 1];
					charIndex += 2;
					retIndex += 2;
				}
				while(charIndex < lineSize && isdigit(line[charIndex])){
					ret[retIndex++] = line[charIndex++];
				}
				// fractional
				if(charIndex < lineSize && (line[charIndex] == '.')){
					ret[retIndex++] = line[charIndex++];
					while(charIndex < lineSize && isdigit(line[charIndex])){
						ret[retIndex++] = line[charIndex++];
					}
				}
				// Exponent
				if(charIndex < lineSize && (line[charIndex] == 'e' || line[charIndex] == 'E' || line[charIndex] == 'p'|| line[charIndex] == 'P')){
					ret[retIndex++] = line[charIndex++];
					// Sign
					if(charIndex < lineSize && (line[charIndex] == '+' || line[charIndex] == '-')){
						ret[retIndex++] = line[charIndex++];
					}
					// digit
					if(!isdigit(line[charIndex])){
						fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
						return NULL;
					}
					while(charIndex < lineSize && isdigit(line[charIndex])){
						ret[retIndex++] = line[charIndex++];
					}
				}
				// Suffix
				if(charIndex < lineSize && (line[charIndex] == 'f' || line[charIndex] == 'l' || line[charIndex] == 'F'|| line[charIndex] == 'L')){
					ret[retIndex++] = line[charIndex++];
				}
			}else if(isalpha(line[charIndex]) || line[charIndex] == '_'){
				// Get identifier
				char *ident = (char *)calloc(65, sizeof(char));
				memset(ident, '\0', 64 * sizeof(char));
				for(int i = 0; i < 64 && (isalnum(line[charIndex]) || line[charIndex] == '_'); ++i, ++charIndex){
					ident[i] = line[charIndex];
				}
				ident = realloc(ident, strlen(ident) + 1);
				MacroInst *macro = NULL;
				if(!mapGet(macroMap, ident, (void **)&macro) && macro->enable){
					if(macro->params == NULL){						
						strcpy(ret + retIndex, macro->str);
						retIndex += strlen(macro->str);
					}else{
						// Generate macro map of parameter
						Map *paramMap = mapNew(compMacro, freeMacroName, freeMacro);
						// Scan parameters
						if(line[charIndex] == '('){
							for(int i = 0; i <= macro->params->size; ++i){
								if(++charIndex > lineSize){
									fprintf(stderr, WASMCC_ERR_INVALID_MACRO_ARGS, getShortName(fileInst), fileInst->curline);
									return NULL;
								}
								char *str = (char *)calloc(4096, sizeof(char));
								for(int j = 0, isStrChr = 0; charIndex < lineSize && (isStrChr || ((i == macro->params->size || line[charIndex] != ',') && line[charIndex] != ')')); ++j, ++charIndex){
									str[j] = line[charIndex];
									if(line[charIndex] == '\"' || line[charIndex] == '\''){
										isStrChr = !isStrChr;
									}
									if(line[charIndex] == '\n'){
										++fileInst->curline;
										fprintf(fout, "\n");
									}
								}
								str = (char *)realloc(str, strlen(str) + 1);
								// Check parameter count
								if(i == macro->params->size - 1){
									if(!macro->hasVA && line[charIndex] != ')'){
										fprintf(stderr, WASMCC_ERR_MACRO_PARAM_TOO_MORE, getShortName(fileInst), fileInst->curline, ident);
										mapFree(&paramMap);
										free(str);
										free(ident);
										return NULL;
									}
								}else if((i < macro->params->size - 1) && (line[charIndex] == ')')){
									fprintf(stderr, WASMCC_ERR_MACRO_PARAM_TOO_FEW, getShortName(fileInst), fileInst->curline, ident);
									mapFree(&paramMap);
									free(str);
									free(ident);
									return NULL;
								}
								// Allocate macro
								MacroInst *newParMacro = malloc(sizeof(MacroInst));
								newParMacro->enable = 1;
								newParMacro->hasVA = 0;
								newParMacro->params = NULL;
								newParMacro->str = str;
								char *keyStr = NULL;
								if(i < macro->params->size){
									char *key = listAt(macro->params, i);
									keyStr = calloc(strlen(key) + 1, sizeof(char));
									strcpy(keyStr, key);
								}else{
									keyStr = calloc(12, sizeof(char));
									strcpy(keyStr, "__VA_ARGS__");
								}
								mapInsert(paramMap, keyStr, newParMacro);
							}
						}else{
							fprintf(stderr, WASMCC_ERR_EXPECT_MACRO_PARAM, getShortName(fileInst), fileInst->curline, ident);
							mapFree(&paramMap);
							free(ident);
							return NULL;
						}
						++charIndex;
						// Expand parameter
						char *expanded = expandMacro(macro->str, paramMap, fileInst, fout);
						strcpy(ret + retIndex, expanded);
						retIndex += strlen(expanded);
						// Clean
						free(expanded);
						mapFree(&paramMap);
					}
					lineRescan = 1;
					macro->enable = 0;
					listAdd(disabledMacro, macro);
				}else{
					strcpy(ret + retIndex, ident);
					retIndex += strlen(ident);
				}
				free(ident);
			}
			ret[retIndex] = line[charIndex];
		}
		if(line != oriLine){
			free(line);
		}
		line = calloc(strlen(ret) + 1, sizeof(char));
		strcpy(line, ret);
		free(ret);
		for(int i = 0; i < disabledMacro->size; ++i){
			MacroInst *macro = listAt(disabledMacro, i);
			macro->enable = 1;
		}
		listFree(&disabledMacro);
	}while(lineRescan == 1);
	return line;
}

static char *getIdent(char *thisCharPtr, FileInst *fileInst, FILE *fout){
	char thisChar = *thisCharPtr;
	char *ident = (char *)calloc(65, sizeof(char));
	if(isalpha(thisChar) || thisChar == '_'){
		memset(ident, '\0', 64 * sizeof(char));
		ident[0] = thisChar;
		for(int i = 1; i <= 64; ++i){
			if(i == 64){
				*thisCharPtr = thisChar;
				return NULL;
			}
			thisChar = nextc(fileInst, fout);
			if(!isalnum(thisChar) && thisChar != '_'){
				break;
			}
			ident[i] = thisChar;
		}
	}else{
		*thisCharPtr = thisChar;
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
	char thisChar = nextc(fileInst, fout);
	for(int i = 0; i < 5; ++i, thisChar = nextc(fileInst, fout)){
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
	// Read line
	char *line = calloc(4096, sizeof(char));
	for(int i = 0; (thisChar = nextc(fileInst, fout)) != '\n'; ++i){
		line[i] = thisChar;
	}
	int lineSize = strlen(line);
	line = realloc(line, lineSize + 1);
	// Expand macro
	char *oldLine = line;
	line = expandMacro(line, macroMap, fileInst, fout);
	free(oldLine);
	// Trim leading space
	int charIndex = 0;
	while(charIndex < lineSize && isspace(line[charIndex])){
		++charIndex;
	}
	// Read header name
	char *headerName = (char *)calloc(4096, sizeof(char));
	if(line[charIndex] == '<'){ // h-char-sequence
		++charIndex;
		for(int i = 0; line[charIndex] != '>'; ++charIndex, ++i){
			if(line[charIndex] == '\n'){
				fprintf(stderr, WASMCC_ERR_H_CHAR_HEADER_NOEND, getShortName(fileInst), fileInst->curline);
				return -1;
			}
			headerName[i] = line[charIndex];
		}
	}else if(line[charIndex] == '\"'){ // q-char-sequence
		++charIndex;
		for(int i = 0; line[charIndex] != '\"'; ++charIndex, ++i){
			if(line[charIndex] == '\n'){
				fprintf(stderr, WASMCC_ERR_Q_CHAR_HEADER_NOEND, getShortName(fileInst), fileInst->curline);
				return -1;
			}
			headerName[i] = line[charIndex];
		}
	}else{
		fprintf(stderr, WASMCC_ERR_EXPECT_HEADER_NAME, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	thisChar = line[charIndex];
	// Get header file path
	char *headerPath = NULL;
	int headerLength = strlen(headerName);
	if(thisChar == '>'){
		headerPath = (char *)calloc(strlen(defInclPath) + headerLength + 2, sizeof(char));
		strcpy(headerPath, defInclPath);
		headerPath[strlen(defInclPath)] = DELIM_CHAR;
		strcat(headerPath, headerName);
	}else{
		char *rchr = strrchr(fileInst->fname, DELIM_CHAR);
		if(rchr){
			int rchrLen = rchr - fileInst->fname;
			headerPath = (char *)calloc(rchrLen + headerLength + 2, sizeof(char));
			memcpy(headerPath, fileInst->fname, rchrLen);
			headerPath[rchrLen] = DELIM_CHAR;
			strcat(headerPath, headerName);
		}else{
			headerPath = (char *)calloc(headerLength + 1, sizeof(char));
			strcpy(headerPath, headerName);
		}
	}
	// Trim trailing space
	++charIndex;
	while(charIndex < lineSize && isspace(line[charIndex]) && line[charIndex] != '\n'){
		++charIndex;
	}
	if(charIndex < lineSize && line[charIndex] != '\n'){
		fprintf(stderr, WASMCC_ERR_CONTAIN_INVALID_CHARACTER, getShortName(fileInst), fileInst->curline);
		return -1;
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
	free(line);
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
	char thisChar = nextc(fileInst, fout);
	for(int i = 0; i < 5; ++i, thisChar = nextc(fileInst, fout)){
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
	thisChar = nextc(fileInst, fout);
	char *macroName = getIdent(&thisChar, fileInst, fout);
	if(!macroName){
		fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	if((!isspace(thisChar) && thisChar != '(')){
		fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Alloc macro instance
	MacroInst *newMacro = (MacroInst *)malloc(sizeof(MacroInst));
	newMacro->hasVA = 0;
	newMacro->enable = 1;
	newMacro->params = NULL;
	// Read parameter
	if(thisChar == '('){
		newMacro->params = listNew();
		thisChar = nextc(fileInst, fout);
		while(thisChar != ')'){
			// Trim leading space
			while(isspace(thisChar) && thisChar != '\n'){
				thisChar = nextc(fileInst, fout);
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
				if((nextc(fileInst, fout)!= '.') || (nextc(fileInst, fout) != '.')){
					fprintf(stderr, WASMCC_ERR_EXPECT_VA, getShortName(fileInst), fileInst->curline);
					free(newMacro);
					listFree(&newMacro->params);
					return -1;
				}
				// Trim trailing space
				while(isspace(thisChar = nextc(fileInst, fout)) && thisChar != '\n');
				if(thisChar != ')'){
					fprintf(stderr, WASMCC_ERR_VA_NOT_END, getShortName(fileInst), fileInst->curline);
					free(newMacro);
					listFree(&newMacro->params);
					return -1;
				}
				newMacro->hasVA = 1;
			}else{
				// Get identifier
				char *ident = getIdent(&thisChar, fileInst, fout);
				if(!ident){
					fprintf(stderr, WASMCC_ERR_INVALID_MACRO_PARAM, getShortName(fileInst), fileInst->curline);
					free(newMacro);
					listFree(&newMacro->params);
					return -1;
				}
				listAdd(newMacro->params, ident);
				// Trim trailing space
				while(isspace(thisChar) && thisChar != '\n'){
					thisChar = nextc(fileInst, fout);
				}
				switch(thisChar){
					case '\n':
						fprintf(stderr, WASMCC_ERR_INVALID_MACRO_PARAM, getShortName(fileInst), fileInst->curline);
						free(newMacro);
						listFree(&newMacro->params);
					return -1;
					case ',':
						thisChar = nextc(fileInst, fout);
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
		thisChar = nextc(fileInst, fout);
	}
	newMacro->str = realloc(macroStr, strlen(macroStr) + 1);
	mapInsert(macroMap, macroName, newMacro);
	return 0;
}
int ppUndef(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	FileInst *fileInst = *fileInstPtr;
	// Check the whole word and next 
	char *word = "ndef"; // "u" has been checked
	char thisChar = nextc(fileInst, fout);
	for(int i = 0; i < 4; ++i, thisChar = nextc(fileInst, fout)){
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
	thisChar = nextc(fileInst, fout);
	char *macroName = getIdent(&thisChar, fileInst, fout);
	if(!macroName){
		fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	if(!isspace(thisChar)){
		fprintf(stderr, WASMCC_ERR_INVALID_IDENTIFIER, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Trim trailing space
	while(thisChar != EOF && isspace(thisChar) && thisChar != '\n'){
		thisChar = nextc(fileInst, fout);
	}
	if(thisChar != EOF && thisChar != '\n'){
		fprintf(stderr, WASMCC_ERR_CONTAIN_INVALID_CHARACTER, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Remove macro
	MacroInst *macro = NULL;
	mapRemove(macroMap, macroName);
	free(macroName);
	return 0;
}
int ppLine(FileInst **fileInstPtr, Stack *fileStack, FILE *fout, Map* macroMap){
	FileInst *fileInst = *fileInstPtr;
	// Check the whole word and next 
	char *word = "ine"; // "l" has been checked
	char thisChar = nextc(fileInst, fout);
	for(int i = 0; i < 3; ++i, thisChar = nextc(fileInst, fout)){
		if(thisChar != word[i]){
			fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
			return -1;
		}
	}
	if(!isspace(thisChar)){
		fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
		return -1;
	}else if(thisChar == '\n'){
		fprintf(stderr, WASMCC_ERR_EXPECT_DIGIT, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Read line
	char *line = calloc(4096, sizeof(char));
	for(int i = 0; (thisChar = nextc(fileInst, fout)) != '\n'; ++i){
		line[i] = thisChar;
	}
	int lineSize = strlen(line);
	line = realloc(line, lineSize + 1);
	// Expand macro
	char *oldLine = line;
	line = expandMacro(line, macroMap, fileInst, fout);
	free(oldLine);
	// Trim leading space
	int charIndex = 0;
	while(charIndex < lineSize && isspace(line[charIndex])){
		++charIndex;
	}
	// Read digit sequence
	if((line[charIndex] == '0' && line[charIndex + 1] == 'x') || isdigit(line[charIndex])){
		++charIndex;
		while(charIndex < lineSize && isdigit(line[charIndex])){
			++charIndex;
		}
		if(!isdigit(line[charIndex]) && !isspace(line[charIndex])){
			fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
			return -1;
		}
		fileInst->curline = atoi(line);
	}else{
		fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Trim leading space
	while(charIndex < lineSize && isspace(line[charIndex]) && line[charIndex] != '\n'){
		++charIndex;
	}
	if(line[charIndex] == '\n'){
		fprintf(fout, "# %d %s", fileInst->curline, fileInst->fname);
		free(line);
		return 0;
	}
	// Read char sequence
	char *headerName = (char *)calloc(4096, sizeof(char));
	if(charIndex < lineSize && line[charIndex] == '\"'){ // q-char-sequence
		++charIndex;
		for(int i = 0; line[charIndex] != '\"'; ++charIndex, ++i){
			if(line[charIndex] == '\n'){
				fprintf(stderr, WASMCC_ERR_Q_CHAR_HEADER_NOEND, getShortName(fileInst), fileInst->curline);
				return -1;
			}
			headerName[i] = line[charIndex];
		}
	}
	headerName = realloc(headerName, strlen(headerName) + 1);
	free(fileInst->fname);
	fileInst->fname = headerName;
	fprintf(fout, "# %d %s", fileInst->curline, fileInst->fname);
	++charIndex;
	// Trim trailing space
	while(charIndex < lineSize && isspace(line[charIndex]) && line[charIndex] != '\n'){
		++charIndex;
	}
	if(charIndex < lineSize && line[charIndex] != '\n'){
		fprintf(stderr, WASMCC_ERR_CONTAIN_INVALID_CHARACTER, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	free(line);
	return 0;
}
int ppPragma(FileInst **fileInstPtr, Stack *fileStack, FILE *fout){
	FileInst *fileInst = *fileInstPtr;
	// Check the whole word and next 
	char *word = "ragma"; // "p" has been checked
	char thisChar = nextc(fileInst, fout);
	for(int i = 0; i < 5; ++i, thisChar = nextc(fileInst, fout)){
		if(thisChar != word[i]){
			fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
			return -1;
		}
	}
	if(!isspace(thisChar)){
		fprintf(stderr, WASMCC_ERR_NON_PP_DIRECTIVE, getShortName(fileInst), fileInst->curline);
		return -1;
	}
	// Skip (Currently no pragma needed, so just skip)
	while(thisChar != EOF && thisChar != '\n'){
		thisChar = nextc(fileInst, fout);
	}
	return 0;
}