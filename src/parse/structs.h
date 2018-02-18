#ifndef WVMCC_PARSE_STRUCTS_DEF
#define WVMCC_PARSE_STRUCTS_DEF

#include "../map.h"
#include "../list.h"

typedef enum {
    Type_Extern = 1,
    Type_Static = 2,
    Type_Thread_local = 4,
    Type_Auto = 8,
    Type_Register = 16
} TypeStorage;

typedef enum {
    Type_Void = 1,
    Type_Char,
    Type_Int,
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

typedef struct {
    Type retType;
    List *params;
} FuncType;

#endif