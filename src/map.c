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
static void assignKeyVal(MapPair *dst, intptr_t key, intptr_t value){
	dst->key = key;
	dst->value = value;
}
static void assignPair(MapPair *dst, MapPair src){
	dst->key = src.key;
	dst->value = src.value;
}
static MapNode *newNode(intptr_t key, intptr_t value){
	MapNode *ret = (MapNode *)malloc(sizeof(MapNode));
	ret->data[0].key = key; 
	ret->data[0].value = value; 
	assignKeyVal(&ret->data[1], (intptr_t)NULL, (intptr_t)NULL);
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
					assignPair(&target->data[1], target->data[0]);
					assignKeyVal(&target->data[0], keyInt, valueInt);
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
					assignKeyVal(&target->data[1], keyInt, valueInt);
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
				assignPair(&target->data[1], target->data[0]);
				target->leaf[2] = target->leaf[1];
				target->leaf[1] = rightNode;
				target->leaf[0] = leftNode;
				leftNode->parent = target;
				rightNode->parent = target;
				assignKeyVal(&target->data[0], keyInt, valueInt);
			}else{
				// Right leaf
				target->leaf[2] = rightNode;
				target->leaf[1] = leftNode;
				leftNode->parent = target;
				rightNode->parent = target;
				assignKeyVal(&target->data[1], keyInt, valueInt);
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
				assignPair(&target->data[0], target->data[1]);
				assignKeyVal(&target->data[1], (intptr_t)NULL, (intptr_t)NULL);
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
				assignKeyVal(&target->data[1], (intptr_t)NULL, (intptr_t)NULL);
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
				assignKeyVal(&target->data[1], (intptr_t)NULL, (intptr_t)NULL);
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
int mapRemove(Map *thisMap, void *key){
	if(thisMap->head == NULL){
		return -1;
	}
	intptr_t keyInt = (intptr_t)key;
	// Traverse to target
	MapNode *target = thisMap->head;
	intptr_t *swapKey = NULL;
	intptr_t *swapValue = NULL;
	while(1){
		int res = thisMap->keyComp(key, (void *)target->data[0].key);
		if(res == -1){
			if(target->leaf[0] == NULL){
				return -1;
			}else{
				target = target->leaf[0];
			}
		}else if(res == 0){
			// Left existed data
			swapKey = &(target->data[0].key);
			swapValue = &(target->data[0].value);
			if(target->leaf[1]){
				target = target->leaf[1];
			}
			break;
		}else{
			if(target->data[1].key != (intptr_t)NULL){
				// 3 node
				res = thisMap->keyComp(key, (void *)target->data[1].key);
				if(res == -1){
					// Middle leaf
					if(target->leaf[1] == NULL){
						return -1;
					}else{
						target = target->leaf[1];
					}
				}else if(res == 0){
					// Right existed data
					swapKey = &(target->data[1].key);
					swapValue = &(target->data[1].value);
					if(target->leaf[2]){
						target = target->leaf[2];
					}
					break;
				}else{
					// Right leaf
					if(target->leaf[2] == NULL){
						return -1;
					}else{
						target = target->leaf[2];
					}
				}
			}else{
				// 2 node
				if(target->leaf[1] == NULL){
					return -1;
				}else{
					target = target->leaf[1];
				}
			}
		}
	}
	// Find replace node
	while(target != NULL && target->leaf[0] != NULL){
		target = target->leaf[0];
	}
	// Swap
	if((*swapKey != target->data[0].key) && (*swapKey != target->data[1].key)){
		(*swapKey) ^= target->data[0].key;
		target->data[0].key = (*swapKey) ^ target->data[0].key;
		(*swapKey) ^= target->data[0].key;
		(*swapValue) ^= target->data[0].value;
		target->data[0].value = (*swapValue) ^ target->data[0].value;
		(*swapValue) ^= target->data[0].value;
	}
	// Remove
	if(target->data[1].key != (intptr_t)NULL){
		// Target is 3 node
		if(target->data[1].key == *swapKey){
			thisMap->keyFree((void *)target->data[1].key);
			thisMap->valueFree((void *)target->data[1].value);
			assignKeyVal(&target->data[1], (intptr_t)NULL, (intptr_t)NULL);
		}else{
			thisMap->keyFree((void *)target->data[0].key);
			thisMap->valueFree((void *)target->data[0].value);
			target->data[0].key = target->data[1].key;
			target->data[0].value = target->data[1].value;
			assignKeyVal(&target->data[1], (intptr_t)NULL, (intptr_t)NULL);
		}
	}else{
		// Target is 2 node
		thisMap->keyFree((void *)target->data[0].key);
		thisMap->valueFree((void *)target->data[0].value);
		assignKeyVal(&target->data[0], (intptr_t)NULL, (intptr_t)NULL);
		MapNode *parent = target->parent;
		MapNode *oriTarget = target;
		while(parent != NULL && parent->data[1].key == (intptr_t) NULL){
			// Parent is 2 node
			if(target == parent->leaf[0]){
				if(parent->leaf[1]->data[1].key != (intptr_t) NULL){
					// Sibling is 3 node
					assignPair(&target->data[0], parent->data[0]);
					assignPair(&parent->data[0], parent->leaf[1]->data[0]);
					assignPair(&parent->leaf[1]->data[0], parent->leaf[1]->data[1]);
					assignKeyVal(&parent->leaf[1]->data[1], (intptr_t) NULL, (intptr_t)NULL);
					target->leaf[1] = parent->leaf[1]->leaf[0];
					if(parent->leaf[1]->leaf[0]){
						parent->leaf[1]->leaf[0]->parent = target;
					}
					parent->leaf[1]->leaf[0] = parent->leaf[1]->leaf[1];
					parent->leaf[1]->leaf[1] = parent->leaf[1]->leaf[2];
					parent->leaf[1]->leaf[2] = NULL;
					if(target != oriTarget){
						free(oriTarget);
					}
					return 0;
				}else{
					assignPair(&parent->leaf[1]->data[1], parent->leaf[1]->data[0]);
					assignPair(&parent->leaf[1]->data[0], parent->data[0]);
					assignKeyVal(&parent->data[0], (intptr_t) NULL, (intptr_t)NULL);
					parent->leaf[1]->leaf[2] = parent->leaf[1]->leaf[1];
					parent->leaf[1]->leaf[1] = parent->leaf[1]->leaf[0];
					parent->leaf[1]->leaf[0] = target->leaf[0];
					if(target->leaf[0]){
						target->leaf[0]->parent = parent->leaf[1];
					}
					parent->leaf[0] = parent->leaf[1];
					parent->leaf[1] = NULL;
				}
			}else{
				if(parent->leaf[1]->data[1].key != (intptr_t) NULL){
					// Sibling is 3 node
					assignPair(&target->data[0], parent->data[0]);
					assignPair(&parent->data[0], parent->leaf[0]->data[1]);
					assignKeyVal(&parent->leaf[0]->data[1], (intptr_t) NULL, (intptr_t)NULL);
					target->leaf[1] = target->leaf[0];
					target->leaf[0] = parent->leaf[0]->leaf[2];
					if(parent->leaf[0]->leaf[2]){
						parent->leaf[0]->leaf[2]->parent = target;
					}
					parent->leaf[0]->leaf[2] = NULL;
					if(target != oriTarget){
						free(oriTarget);
					}
					return 0;
				}else{
					assignPair(&parent->leaf[0]->data[1], parent->data[0]);
					assignKeyVal(&parent->data[0], (intptr_t) NULL, (intptr_t)NULL);
					parent->leaf[0]->leaf[2] = target->leaf[0];
					if(target->leaf[0]){
						target->leaf[0]->parent = parent->leaf[0];
					}
				}
			}
			target = parent;
			parent = parent->parent;
		}
		if(parent != NULL && parent->data[0].key != (intptr_t)NULL){
			// Parent is 3 node
			if(target == parent->leaf[0]){
				if(parent->leaf[1]->data[1].key != (intptr_t) NULL){
					// Sibling is 3 node
					assignPair(&target->data[0], parent->data[0]);
					assignPair(&parent->data[0], parent->leaf[1]->data[0]);
					assignPair(&parent->leaf[1]->data[0], parent->leaf[1]->data[1]);
					assignKeyVal(&parent->leaf[1]->data[1], (intptr_t) NULL, (intptr_t)NULL);
					target->leaf[1] = parent->leaf[1]->leaf[0];
					if(parent->leaf[1]->leaf[0]){
						parent->leaf[1]->leaf[0]->parent = target;
					}
					parent->leaf[1]->leaf[0] = parent->leaf[1]->leaf[1];
					parent->leaf[1]->leaf[1] = parent->leaf[1]->leaf[2];
					parent->leaf[1]->leaf[2] = NULL;
				}else{
					assignPair(&parent->leaf[1]->data[1], parent->leaf[1]->data[0]);
					assignPair(&parent->leaf[1]->data[0], parent->data[0]);
					assignPair(&parent->data[0], parent->data[1]);
					assignKeyVal(&parent->data[1], (intptr_t) NULL, (intptr_t)NULL);
					parent->leaf[1]->leaf[2] = parent->leaf[1]->leaf[1];
					parent->leaf[1]->leaf[1] = parent->leaf[1]->leaf[0];
					parent->leaf[1]->leaf[0] = target->leaf[0];
					if(target->leaf[0]){
						target->leaf[0]->parent = parent->leaf[1];
					}
					parent->leaf[0] = parent->leaf[1];
					parent->leaf[1] = parent->leaf[2];
					parent->leaf[2] = NULL;
				}
			}else if(target == parent->leaf[2]){
				if(parent->leaf[1]->data[1].key != (intptr_t) NULL){
					// Sibling is 3 node
					assignPair(&target->data[0], parent->data[1]);
					assignPair(&parent->data[1], parent->leaf[1]->data[1]);
					assignKeyVal(&parent->leaf[1]->data[1], (intptr_t) NULL, (intptr_t)NULL);
					target->leaf[1] = target->leaf[0];
					target->leaf[0] = parent->leaf[1]->leaf[2];
					if(parent->leaf[1]->leaf[2]){
						parent->leaf[1]->leaf[2]->parent = target;
					}
					parent->leaf[1]->leaf[2] = NULL;
					return 0;
				}else{
					assignPair(&parent->leaf[1]->data[1], parent->data[1]);
					assignKeyVal(&parent->data[1], (intptr_t) NULL, (intptr_t)NULL);
					parent->leaf[1]->leaf[2] = target->leaf[0];
					if(target->leaf[0]){
						target->leaf[0]->parent = parent->leaf[1];
					}
					parent->leaf[2] = NULL;
				}
			}else{
				if(parent->leaf[0]->data[1].key != (intptr_t) NULL){
					// Sibling is 3 node
					assignPair(&target->data[0], parent->data[0]);
					assignPair(&parent->data[0], parent->leaf[0]->data[1]);
					assignKeyVal(&parent->leaf[0]->data[1], (intptr_t) NULL, (intptr_t)NULL);
					target->leaf[1] = target->leaf[0];
					target->leaf[0] = parent->leaf[0]->leaf[2];
					if(parent->leaf[0]->leaf[2]){
						parent->leaf[0]->leaf[2]->parent = target;
					}
					parent->leaf[0]->leaf[2] = NULL;
				}else if(parent->leaf[2]->data[1].key != (intptr_t) NULL){
					// Sibling is 3 node
					assignPair(&target->data[0], parent->data[1]);
					assignPair(&parent->data[1], parent->leaf[2]->data[0]);
					assignPair(&parent->leaf[2]->data[0], parent->leaf[2]->data[1]);
					assignKeyVal(&parent->leaf[2]->data[1], (intptr_t) NULL, (intptr_t)NULL);
					target->leaf[1] = parent->leaf[2]->leaf[0];
					if(parent->leaf[2]->leaf[0]){
						parent->leaf[2]->leaf[0]->parent = target;
					}
					parent->leaf[2]->leaf[0] = parent->leaf[2]->leaf[1];
					parent->leaf[2]->leaf[1] = parent->leaf[2]->leaf[2];
					parent->leaf[2]->leaf[2] = NULL;
				}else{
					assignPair(&parent->leaf[2]->data[1], parent->leaf[2]->data[0]);
					assignPair(&parent->leaf[2]->data[0], parent->data[1]);
					assignKeyVal(&parent->data[1], (intptr_t) NULL, (intptr_t)NULL);
					parent->leaf[2]->leaf[2] = parent->leaf[2]->leaf[1];
					parent->leaf[2]->leaf[1] = parent->leaf[2]->leaf[0];
					parent->leaf[2]->leaf[0] = target->leaf[0];
					if(target->leaf[0]){
						target->leaf[0]->parent = parent->leaf[2];
					}
					parent->leaf[1] = parent->leaf[2];
					parent->leaf[2] = NULL;
				}
			}
		}else{
			// Parent is null node
			thisMap->head = target->leaf[0];
			if(target->leaf[0]){
				target->leaf[0]->parent = NULL;
			}
			free(target);
		}
		free(oriTarget);
	}
	return 0;
}
int mapGet(Map *thisMap, void *key, void** dataPtr){
	for(MapNode *cur = thisMap->head; cur != NULL; ){
		int res = thisMap->keyComp(key, (void *)cur->data[0].key);
		if(res == -1){
			cur = cur->leaf[0];
		}else if(res == 0){
			// Left existed data
			*dataPtr = (void *)cur->data[0].value;
			return 0;
		}else{
			if(cur->data[1].key != (intptr_t)NULL){
				// 3 node
				res = thisMap->keyComp(key, (void *)cur->data[1].key);
				if(res == -1){
					// Middle leaf
					cur = cur->leaf[1];
				}else if(res == 0){
					// Right existed data
					*dataPtr = (void *)cur->data[1].value;
					return 0;
				}else{
					// Right leaf
					cur = cur->leaf[2];
				}
			}else{
				// 2 node
				cur = cur->leaf[1];
			}
		}
	}
	*dataPtr = NULL;
	return -1;
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
	free(*thisMapPtr);
	thisMapPtr = NULL;
}