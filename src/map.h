#ifndef WASMCC_MAP_DEF
#define WASMCC_MAP_DEF

#include <stdint.h>
#include <stdlib.h>

typedef struct {
  intptr_t key;
  intptr_t value;
} MapPair;

typedef struct MapNode_ {
  MapPair data[2];
  struct MapNode_* leaf[3];
  struct MapNode_* parent;
} MapNode;

// Use this to compare key, -1: less , 0: equal, 1: greater
typedef int (*keyComp_t)(void*, void*);
// Use this to free key or value
typedef void (*pairFree_t)(void*);

typedef struct {
  keyComp_t keyComp;
  pairFree_t keyFree;
  pairFree_t valueFree;
  MapNode* head;
  unsigned int size;
} Map;
// mapNew: Create and initialize new map
Map* mapNew(keyComp_t keyComp, pairFree_t keyFree, pairFree_t valueFree);
// mapInsert: If key doesn't exist, insert it, or free the existed data and set
// the data for this existed key
void mapInsert(Map* thisMap, void* key, void* data);
// mapRemove: Remove and free the key and value. Return 0 on success, or return
// -1 if not exist
int mapRemove(Map* thisMap, void* key);
// mapGet: Get the data of existed key, or return -1
int mapGet(Map* thisMap, void* key, void** dataPtr);
// mapFree: Free all data and the map;
void mapFree(Map** thisMapPtr);

#endif