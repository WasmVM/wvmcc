#include <adt/Stack.h>

Stack* stackNew() {
  Stack* newStack = (Stack*)malloc(sizeof(Stack));
  newStack->head = NULL;
  newStack->size = 0;
  return newStack;
}
void stackPush(Stack* thisStack, void* data) {
  StackNode* newNode = (StackNode*)malloc(sizeof(StackNode));
  newNode->data = data;
  newNode->next = thisStack->head;
  thisStack->head = newNode;
  ++thisStack->size;
}
int stackPop(Stack* thisStack, void** dataPtr) {
  if (thisStack->size) {
    StackNode* node = thisStack->head;
    thisStack->head = node->next;
    *dataPtr = node->data;
    free(node);
    --thisStack->size;
    return 0;
  } else {
    return -1;
  }
}
int stackTop(Stack* thisStack, void** dataPtr) {
  if (thisStack->size) {
    *dataPtr = thisStack->head->data;
    return 0;
  } else {
    return -1;
  }
}
void stackFree(Stack** thisStackPtr) {
  free(*thisStackPtr);
  *thisStackPtr = NULL;
}