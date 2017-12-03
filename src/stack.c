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

#include "stack.h"

Stack *stackNew(){
	Stack *newStack = (Stack *)malloc(sizeof(Stack));
	newStack->head = NULL;
	newStack->size = 0;
	return newStack;
}
void stackPush(Stack *thisStack, void *data){
	StackNode *newNode = (StackNode *)malloc(sizeof(StackNode));
	newNode->data = data;
	newNode->next = thisStack->head;
	thisStack->head = newNode;
	++thisStack->size;
}
int stackPop(Stack *thisStack, void **dataPtr){
	if(thisStack->size){
		StackNode *node = thisStack->head;
		thisStack->head = node->next;
		*dataPtr = node->data;
		--thisStack->size;
		return 1;
	}else{
		return 0;
	}
}
int stackTop(Stack *thisStack, void **dataPtr){
	if(thisStack->size){
		*dataPtr = thisStack->head->data;
		return 1;
	}else{
		return 0;
	}
}
void stackFree(Stack **thisStackPtr){
	free(*thisStackPtr);
	*thisStackPtr = NULL;
}