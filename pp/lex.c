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

#include "lex.h"

// Private other token
static PPToken * otherToken(PPLexer *this){
	PPToken *newToken = (PPToken *)malloc(sizeof(PPToken));
	char *begin = this->cur;
	this->cur = memchr(this->cur, '\n', this->codeSize - (this->cur - this->code));
	if(!this->cur){
		this->cur = this->code + this->codeSize;
	}
	newToken->type = PPOther;
	newToken->line = this->curLine;
	newToken->pos = this->curPos;
	size_t strSize = this->cur - begin;
	newToken->str = (char *)calloc(strSize + 1, sizeof(char));
	memcpy(newToken->str, begin, strSize);
	this->curPos += strSize;
	return newToken; 
}

// Private punctuator token
static PPToken *puncToken(PPLexer *this){
	// Save cur
	char *cur = this->cur;
	unsigned int curSize = this->cur - this->code;
	char *puncStr = NULL;
	// Check punctuator
	switch(*cur){
		// One-state
		case '[':
		case ']':
		case '(':
		case ')':
		case '{':
		case '}':
		case '~':
		case '?':
		case ';':
		case ',':
			puncStr = (char *)calloc(2, sizeof(char));
			puncStr[0] = *cur++;
		break;
		// Two-states
		case '*':
		case '!':
		case '/':
		case '=':
		case '^':
			if((curSize + 1 <= this->codeSize) && (cur[1] == '=')){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = cur[0];
				puncStr[1] = '=';
				cur += 2;
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = *cur++;
			}
		break;
		case '.':
			if((curSize + 2 <= this->codeSize) && (cur[1] == '.' && cur[2] == '.')){
				puncStr = (char *)calloc(4, sizeof(char));
				puncStr[0] = '.';
				puncStr[1] = '.';
				puncStr[2] = '.';
				cur += 3;
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = '.';
				cur++;
			}
		break;
		case '#':
			if((curSize + 1 <= this->codeSize) && (cur[1] == '#')){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = '#';
				puncStr[1] = '#';
				cur += 2;
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = '#';
				cur++;
			}
		break;
		case ':':
			if((curSize + 1 <= this->codeSize) && cur[1] == '>'){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = ':';
				puncStr[1] = '>';
				cur += 2;
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = ':';
				cur++;
			}
		break;
		// Three-states
		case '+':
		case '&':
		case '|':
			if((curSize + 1 <= this->codeSize) && (cur[0] == cur[1] || cur[1] == '=')){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = cur[0];
				puncStr[1] = cur[1];
				cur += 2;
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = *cur++;
			}
		break;
		case '-':
			if((curSize + 1 <= this->codeSize) && (cur[1] == '-' || cur[1] == '=' || cur[1] == '>')){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = cur[0];
				puncStr[1] = cur[1];
				cur += 2;
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = *cur++;
			}
		break;
		case '<':
			if((curSize + 1 <= this->codeSize) && (cur[1] == '=' || cur[1] == ':' || cur[1] == '%')){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = '<';
				puncStr[1] = cur[1];
				cur += 2;
			}else if((curSize + 1 <= this->codeSize) && (cur[1] == '<')){
				if((curSize + 2 <= this->codeSize) && (cur[2] == '=')){
					puncStr = (char *)calloc(4, sizeof(char));
					puncStr[0] = '<';
					puncStr[1] = '<';
					puncStr[2] = '=';
					cur += 3;
				}else{
					puncStr = (char *)calloc(3, sizeof(char));
					puncStr[0] = '<';
					puncStr[1] = '<';
					cur += 2;
				}
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = '<';
				cur++;
			}
		break;
		case '>':
			if((curSize + 1 <= this->codeSize) && (cur[1] == '=')){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = '>';
				puncStr[1] = '=';
				cur += 2;
			}else if((curSize + 1 <= this->codeSize) && (cur[1] == '>')){
				if((curSize + 2 <= this->codeSize) && (cur[2] == '=')){
					puncStr = (char *)calloc(4, sizeof(char));
					puncStr[0] = '>';
					puncStr[1] = '>';
					puncStr[2] = '=';
					cur += 3;
				}else{
					puncStr = (char *)calloc(3, sizeof(char));
					puncStr[0] = '>';
					puncStr[1] = '>';
					cur += 2;
				}
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = '>';
				cur++;
			}
		break;
		case '%':
			if((curSize + 1 <= this->codeSize) && (cur[1] == '=' || cur[1] == ':' || cur[1] == '>')){
				puncStr = (char *)calloc(3, sizeof(char));
				puncStr[0] = '%';
				puncStr[1] = cur[1];
				cur += 2;
			}else if((curSize + 3 <= this->codeSize) && (cur[0] == cur[2]) && (cur[1] == ':') && (cur[3] == ':')){
				puncStr = (char *)calloc(5, sizeof(char));
				puncStr[0] = '%';
				puncStr[1] = ':';
				puncStr[2] = '%';
				puncStr[3] = ':';
				cur += 4;
			}else{
				puncStr = (char *)calloc(2, sizeof(char));
				puncStr[0] = '%';
				cur++;
			}
		break;
		default:
		break;
	}
	// Allocate token
	if(puncStr){
		PPToken *newToken = (PPToken *)malloc(sizeof(PPToken));
		newToken->type = PPPunct;
		newToken->line = this->curLine;
		newToken->pos = this->curPos;
		newToken->str = puncStr;
		this->cur = cur;
		return newToken;
	}
	return NULL;
}

// Private trim whitespace
static void trimSpace(PPLexer *this){
	while(isspace(*this->cur)){
		if(*this->cur == '\n'){
			this->curPos = 0;
			this->curLine += 1;
		}else{
			++this->curPos;
		}
		++this->cur;
	}
}

// Public
static PPToken *nextToken(PPLexer *this){
	// Check if EOF
	if((this->cur - this->code) >= this->codeSize){
		return NULL;
	}

	// Trim space before comment
	trimSpace(this);

	// Skip line comment
	while(this->cur[0] == '/' && this->cur[1] == '/'){
		while(*(this->cur++) != '\n');
		trimSpace(this);
	}

	// Skip block comment
	while(this->cur[0] == '/' && this->cur[1] == '*'){
		for(this->cur += 2; (this->cur[0] != '*') || (this->cur[1] != '/'); ++this->cur);
		this->cur += 2;
		trimSpace(this);
	}

	char firstCh = *this->cur;
	
	if(isalpha(firstCh) || firstCh == '_'){ // Identifier
		
	}else if(firstCh == '\''){ // Character-constant

	}else if(firstCh == '\"'){ // String-literal

	}else if(isdigit(firstCh)){ // pp-number (digit only)
		
	}
	// Punctuator and other
	PPToken *punc = puncToken(this);
	if(punc){
		return punc;
	}else{
		return otherToken(this);
	}
}

// Public destructor
void freePPLexer(PPLexer **lexer){
	if(lexer){
		free((*lexer)->filename);
		free((*lexer)->code);
		free(*lexer);
		*lexer = NULL;
	}
}

// Public constructor
PPLexer *initPPLexer(const char *fileName){
	// Open file
	FILE *fin = fopen(fileName, "rb");
	if(!fin){
		perror("[Wasmcpp lexer]");
		return NULL;
	}
	// Allocate lexer
	PPLexer *lexer = (PPLexer *)malloc(sizeof(PPLexer));
	lexer->nextToken = nextToken;
	lexer->curLine = 1;
	lexer->curPos = 0;
	lexer->filename = (char *)calloc(strlen(fileName) + 1, sizeof(char));
	memcpy(lexer->filename, fileName, strlen(fileName));
	// Read file
	fseek(fin, 0, SEEK_END);
	lexer->codeSize = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	lexer->code = (char *)malloc(lexer->codeSize);
	fread(lexer->code, 1, lexer->codeSize, fin);
	fclose(fin);
	lexer->cur = lexer->code;
	return lexer;
}