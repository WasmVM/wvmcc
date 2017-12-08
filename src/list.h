#ifndef WASMCC_LIST_DEF
#define WASMCC_LIST_DEF

#include <stdlib.h>

typedef struct ListNode_ {
    void *data;
    struct ListNode_ *next;
} ListNode;

typedef struct List_ {
    ListNode *head;
	ListNode *tail;
    unsigned int size;
} List;

// Allocate new list
List *listNew();
// Add value to the tail of list
void listAdd(List *theList, void *val);
// Return value if success, or NULL if failed
void *listAt(List *theList, int index);
// Return index of value if success, or -1 if failed
int listIndexOf(List *theList, void *val);
// Return value if success, or NULL if failed
void *listRemove(List *theList, int index);
// Free list and set to NULL
void listFree(List **theListPtr);

#endif