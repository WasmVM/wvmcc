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

#include "lex.h"

int main(int argc, char *argv[]){
	// Lexer
	PPLexer *lexer = initPPLexer(argv[1]);
	if(!lexer){
		return -1;
	}
	// Print token
	PPToken *tok = NULL;
	while((tok = lexer->nextToken(lexer))){
		switch (tok->type){
			case PPHead:
				printf("%u:%u <header-name> %s\n", tok->line, tok->pos, tok->str);
			break;
			case PPOther:
				printf("%u:%u <other> %s\n", tok->line, tok->pos, tok->str);
			break;
			case PPPunct:
				printf("%u:%u <punctuator> %s\n", tok->line, tok->pos, tok->str);
			break;
			default:
				printf("%u:%u <Unknown> %s\n", tok->line, tok->pos, tok->str);
			break;
		}
		freePPToken(&tok);
	}

	// Clean
	freePPLexer(&lexer);
	return 0;
}