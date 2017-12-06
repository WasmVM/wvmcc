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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/map.h"

int compKey(void *aPtr, void* bPtr){
	int a = *((int *)aPtr);
	int b = *((int *)bPtr);
	return (a > b) - (a < b);
}

void freeKey(void *ptr){
	int *keyPtr = (int *)ptr;
	free(keyPtr);
	ptr = NULL;
}

void freeValue(void *ptr){
	char *charPtr = (char *)ptr;
	free(charPtr);
	ptr = NULL;
}

int newId(){
	static int id = 0;
	return id++;
}

void printTree(MapNode *node, FILE *fout, int id){
	// Print this node
	if(node == NULL){
		fprintf(fout, "node%d[label = \"\"];\n", id);
		return;
	}
	if(node->data[1].key){
		fprintf(fout, "node%d[label = \"<f0> |%d:%s|<f1> |%d:%s|<f2>\"];\n",
			id,
			*((int *)node->data[0].key), 
			(char *)node->data[0].value,
			*((int *)node->data[1].key),
			(char *)node->data[1].value
		);
	}else{
		fprintf(fout, "node%d[label = \"<f0> |%d:%s|<f1> | |<f2>\"];\n",
			id,
			*((int *)node->data[0].key), 
			(char *)node->data[0].value
		);
	}
	// Print transition
	for(int i = 0; i < 3; ++i){
		if(node->leaf[i]){
			int leafId = newId();
			printTree(node->leaf[i], fout, leafId);
			fprintf(fout, "\"node%d\":f%d -> \"node%d\";\n", id, i, leafId);
		}
	}
}

int main(void){
	Map *map = mapNew(compKey, freeKey, freeValue);
	// Input
	printf("[Format: +%%d%%20s to insert, -%%d to delete,^ to end]\n> ");
	char op;
	while((op = fgetc(stdin)) != '^'){
		switch(op){
			case '+': {
				int *key = (int *)malloc(sizeof(int));
				char *str = (char *)calloc(20, sizeof(char));
				scanf("%d%s", key, str);
				mapInsert(map, key, str);
				// Print tree
				FILE *fout = fopen("out.dot", "w");
				fprintf(fout, "digraph{node [shape = record,height=.1];\n");
				printTree(map->head, fout, newId());
				fprintf(fout, "\n}");
				fclose(fout);
			}break;
			case '-':{
				int *key = (int *)malloc(sizeof(int));
				scanf("%d", key);
				if(mapRemove(map, key) == -1){
					printf("The key is not found.\n");
				}else{
					// Print tree
					FILE *fout = fopen("out.dot", "w");
					fprintf(fout, "digraph{node [shape = record,height=.1];\n");
					printTree(map->head, fout, newId());
					fprintf(fout, "\n}");
					fclose(fout);
				}
				free(key);
			}break;
			case '\n': 
				printf("> ");
			break;
			default:
				printf("Unknown operator, valid operator is +, - and ^\n");
			break;
		}
	}
	// Clean
	mapFree(&map);
	return 0;
}