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

#include "list.h"

List* listNew() {
  List* ret = (List*)malloc(sizeof(List));
  ret->size = 0;
  ret->head = NULL;
  ret->tail = NULL;
  return ret;
}
void listAdd(List* theList, void* val) {
  ListNode* newNode = (ListNode*)malloc(sizeof(List));
  newNode->next = NULL;
  newNode->data = val;
  ++theList->size;
  if (theList->head == NULL) {
    theList->head = newNode;
    theList->tail = newNode;
  } else {
    theList->tail->next = newNode;
    theList->tail = newNode;
  }
}
void* listAt(List* theList, int index) {
  if (index >= theList->size) {
    return NULL;
  }
  ListNode* cur = theList->head;
  while (index-- > 0) {
    cur = cur->next;
  }
  return cur->data;
}
int listIndexOf(List* theList, void* val) {
  ListNode* cur = theList->head;
  for (int i = 0; i < theList->size; ++i, cur = cur->next) {
    if (cur->data == val) {
      return i;
    }
  }
  return -1;
}
void* listRemove(List* theList, int index) {
  if (index < 0 || index >= theList->size || theList->size == 0) {
    return NULL;
  }
  --theList->size;
  ListNode* curHead = NULL;
  if (index == 0) {
    void* data = (theList->head) ? theList->head->data : NULL;
    curHead = theList->head;
    theList->head = (theList->head) ? theList->head->next : NULL;
	theList->tail = (theList->head) ? theList->head->next : NULL;
    free(curHead);
    return data;
  }
  ListNode* prev = theList->head;
  ListNode* cur = theList->head->next;
  while (--index > 0) {
    prev = cur;
    cur = cur->next;
  }
  if (cur == theList->tail) {
    theList->tail = prev;
  }
  prev->next = cur->next;
  void *data = cur->data;
  free(cur);
  return data;
}
void listFree(List** theListPtr) {
  if (*theListPtr == NULL) {
    return;
  }
  ListNode* cur = (*theListPtr)->head;
  while (cur != NULL) {
    ListNode* node = cur;
    cur = cur->next;
    free(node);
  }
  free(*theListPtr);
  theListPtr = NULL;
}