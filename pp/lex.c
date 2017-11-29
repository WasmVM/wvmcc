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

static PPToken *nextToken(){
	return NULL;
}

void freePPLexer(PPLexer **lexer){
	if(lexer != NULL){
		free((*lexer)->code);
		free(*lexer);
		*lexer = NULL;
	}
}

PPLexer *initPPLexer(const char *fileName){
	PPLexer *lexer = (PPLexer *)malloc(sizeof(PPLexer));
	lexer->nextToken = nextToken;
	return lexer;
}