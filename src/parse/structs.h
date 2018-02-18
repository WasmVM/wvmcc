#ifndef WVMCC_PARSE_STRUCTS_DEF
#define WVMCC_PARSE_STRUCTS_DEF

#include "../map.h"
#include "../list.h"

typedef enum {
    Type_Auto = 1,
    Type_Extern,
    Type_Static,
    Type_Register,
    Type_Typedef,
    Type_Thread_local = 8,
} TypeStorage;

typedef enum {
    Type_Int = 1,
    Type_Void,
    Type_Char,
    Type_float,
    Type_Struct,
    Type_Enum,
    Type_Union,
    Type_Bool,
    Type_Complex,
    Type_Short,
    Type_Long,
    Type_Longlong
} TypeSpecfier;

typedef enum {
    Type_Unsigned = 1,
    Type_Atomic = 2,
    Type_Pointer = 4,
    Type_Function = 8
} TypeOptional;

typedef struct {
    char storage;
    char sprcifier;
    char optional;
} Type;

void initType(Type *type);

typedef struct {
    Type retType;
    List *params;
} FuncType;

#endif