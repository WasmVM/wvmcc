#ifndef WASMCC_STACK_DEF
#define WASMCC_STACK_DEF

#include <stdlib.h>

typedef struct StackNode_ {
  void* data;
  struct StackNode_* next;
} StackNode;

typedef struct {
  StackNode* head;
  unsigned int size;
} Stack;

Stack* stackNew();
void stackPush(Stack* thisStack, void* data);
// If success return 0, or return -1
int stackPop(Stack* thisStack, void** dataPtr);
int stackTop(Stack* thisStack, void** dataPtr);
void stackFree(Stack** thisStackPtr);

#endif  // !WASMCC_STACK_DEF