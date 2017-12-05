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

#include "map.h"

Map *mapNew(keyComp_t keyComp, pairFree_t keyFree, pairFree_t valueFree){
	Map *newMap = (Map *)malloc(sizeof(Map));
	newMap->head = NULL;
	newMap->size = 0;
	newMap->keyComp = keyComp;
	newMap->keyFree = keyFree;
	newMap->valueFree = valueFree;
	return newMap;
}
static MapNode *newNode(intptr_t key, intptr_t value){
	MapNode *ret = (MapNode *)malloc(sizeof(MapNode));
	ret->data[0].key = key; 
	ret->data[1].key = (intptr_t)NULL; 
	ret->data[0].value = value; 
	ret->data[1].value = (intptr_t)NULL;
	for(int i = 0; i < 3; ++i){
		ret->leaf[i] = NULL;
	}
	ret->parent = NULL;
	return ret;
}

void mapInsert(Map *thisMap, void* key, void* value){
	intptr_t keyInt = (intptr_t)key;
	intptr_t valueInt = (intptr_t)value;
	// Head
	if(thisMap->head == NULL){
		thisMap->head = newNode(keyInt, valueInt);
		return;
	}
	// Traverse to the leaf
	MapNode *target = thisMap->head;
	int cmpres = 0;
	while(1){
		int res = thisMap->keyComp(key, (void *)target->data[0].key);
		if(res == -1){
			if(target->leaf[0] == NULL){
				if(target->data[1].key == (intptr_t)NULL){
					// 2 node
					target->data[1].key = target->data[0].key;
					target->data[1].value = target->data[0].value;
					target->data[0].key = keyInt;
					target->data[0].value = valueInt;
					return;
				}else{
					// Left leaf
					cmpres = -1;
					break;
				}
			}else{
				target = target->leaf[0];
			}
		}else if(res == 0){
			// Left existed data
			thisMap->valueFree((void *)target->data[0].value);
			target->data[0].value = valueInt;
			return;
		}else{
			if(target->data[1].key != (intptr_t)NULL){
				// 3 node
				res = thisMap->keyComp(key, (void *)target->data[1].key);
				if(res == -1){
					// Middle leaf
					if(target->leaf[1] == NULL){
						cmpres = 0;
						break;
					}else{
						target = target->leaf[1];
					}
				}else if(res == 0){
					// Right existed data
					thisMap->valueFree((void *)target->data[1].value);
					target->data[1].value = valueInt;
					return;
				}else{
					// Right leaf
					if(target->leaf[2] == NULL){
						cmpres = 1;
						break;
					}else{
						target = target->leaf[2];
					}
				}
			}else{
				// 2 node
				if(target->leaf[1] == NULL){
					target->data[1].key = keyInt;
					target->data[1].value = valueInt;
					return;
				}else{
					target = target->leaf[1];
				}
			}
		}
	}
	// Swap value
	void *tmp = NULL;
	switch(cmpres){
		case -1:
			target->data[0].key ^= keyInt;
			keyInt = target->data[0].key ^ keyInt;
			target->data[0].key ^= keyInt;
			target->data[0].value ^= valueInt;
			valueInt = target->data[0].value ^ valueInt;
			target->data[0].value ^= valueInt;
		break;
		case 1:
			target->data[1].key ^= keyInt;
			keyInt = target->data[1].key ^ keyInt;
			target->data[1].key ^= keyInt;
			target->data[1].value ^= valueInt;
			valueInt = target->data[1].value ^ valueInt;
			target->data[1].value ^= valueInt;
		break;
		default: 
		break;
	}
	// Create new node
	MapNode *rightNode = newNode(target->data[1].key, target->data[1].value);
	MapNode *leftNode = target;
	target->data[1].key = (intptr_t)NULL;
	target->data[1].value = (intptr_t)NULL;
	MapNode *curNode = target;
	// Split parent
	for(target = target->parent; target != NULL; curNode = target, target = target->parent){
		if(target->data[1].key == (intptr_t)NULL){
			// 2 node
			if(target->leaf[0] == curNode){
				// Left leaf
				target->data[1].key = target->data[0].key;
				target->data[1].value = target->data[0].value;
				target->leaf[2] = target->leaf[1];
				target->leaf[1] = rightNode;
				target->leaf[0] = leftNode;
				leftNode->parent = target;
				rightNode->parent = target;
				target->data[0].key = keyInt;
				target->data[0].value = valueInt;
			}else{
				// Right leaf
				target->leaf[2] = rightNode;
				target->leaf[1] = leftNode;
				leftNode->parent = target;
				rightNode->parent = target;
				target->data[1].key = keyInt;
				target->data[1].value = valueInt;
			}
			return;
		}else{
			// 3 node
			if(target->leaf[0] == curNode){
				// Left leaf
				MapNode *newLeft = newNode(keyInt, valueInt);
				newLeft->leaf[0] = leftNode;
				newLeft->leaf[1] = rightNode;
				leftNode->parent = newLeft;
				rightNode->parent = newLeft;
				leftNode = newLeft;
				rightNode = target;
				keyInt = target->data[0].key;
				valueInt = target->data[0].value;
				target->data[0].key = target->data[1].key;
				target->data[0].value = target->data[1].value;
				target->data[1].key = (intptr_t)NULL;
				target->data[1].value = (intptr_t)NULL;
				target->leaf[0] = target->leaf[1];
				target->leaf[1] = target->leaf[2];
				target->leaf[2] = NULL;
			}else if(target->leaf[1] == curNode){
				// Middle leaf
				MapNode *newRight = newNode(target->data[1].key, target->data[1].value);
				newRight->leaf[0] = rightNode;
				newRight->leaf[1] = target->leaf[2];
				rightNode->parent = newRight;
				target->leaf[2]->parent = newRight;
				target->leaf[1] = leftNode;
				leftNode = target;
				rightNode = newRight;
				target->data[1].key = (intptr_t)NULL;
				target->data[1].value = (intptr_t)NULL;
				target->leaf[2] = NULL;
			}else{
				// Right leaf
				MapNode *newRight = newNode(keyInt, valueInt);
				newRight->leaf[0] = leftNode;
				newRight->leaf[1] = rightNode;
				leftNode->parent = newRight;
				rightNode->parent = newRight;
				leftNode = target;
				rightNode = newRight;
				keyInt = target->data[1].key;
				valueInt = target->data[1].value;
				target->data[1].key = (intptr_t)NULL;
				target->data[1].value = (intptr_t)NULL;
				target->leaf[2] = NULL;
			}
		}
	}
	// New head
	thisMap->head = newNode(keyInt, valueInt);
	thisMap->head->leaf[0] = leftNode;
	thisMap->head->leaf[1] = rightNode;
	leftNode->parent = thisMap->head;
	rightNode->parent = thisMap->head;
}
int mapRemove(Map *thisMap, void *Key){
	return 0;
}
int mapGet(Map *thisMap, void *key, void** dataPtr){
	return 0;
}
static void freeNode(Map *this, MapNode *node){
	if(node == NULL){
		return;
	}
	// Free leaves
	for(int i = 0; i < 3; ++i){
		freeNode(this, node->leaf[i]);
	}
	// Free pairs
	for(int i = 0; i < 2; ++i){
		this->keyFree((void *)node->data[i].key);
		this->valueFree((void *)node->data[i].value);
	}
	free(node);
}

void mapFree(Map **thisMapPtr){
	freeNode(*thisMapPtr, (*thisMapPtr)->head);
	thisMapPtr = NULL;
}