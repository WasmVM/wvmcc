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
    Type_Short,
} TypeSpecfier;

typedef enum {
    Type_Atomic = 1,
    Type_Const = 2,
    Type_Restrict = 4,
    Type_Volatile = 8,
    Type_Unsigned = 16,
    Type_Long = 32,
    Type_Longlong = 64,
    Type_Complex = 128
} TypeQualifier;

typedef enum {
    Array_Static = 16,
    Array_Variable = 32,
    Array_Unspecified = 64
} ArrayQualifier;

typedef enum {
    Tail_Array = 1,
    Tail_Function = 2,
    Tail_Function_va = 3,
    Tail_Struct = 4;
} DeclaratorTailType;

typedef struct {
    char tailType;
    char qualifier;
    union{
        char *identifier;
        int size;
    }length;
} ArrayDeclarator;

typedef struct {
    char *pointers;
    char ptrSize;
    char *identifier;
    List *list; // List of array and param declatators
} Declarator;

typedef struct {
    char storage;
    char specifier;
    char qualifier;
    Declarator declarator;
} Declaration;

void initDeclaration(Declaration *decl);
void freeDeclaration(Declaration *decl);

typedef struct {
    Declaration decl;
    int bitField;
} StructUnionProps;

typedef struct {
    Declaration decl;
    List *props; // List of struct declatators
} StructUnion;

#endif