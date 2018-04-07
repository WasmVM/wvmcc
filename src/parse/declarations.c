#include "rules.h"
#include "util.h"

int declaration(FileInst* fInst, Map* typeMap, List* declList) {
  long int fpos = ftell(fInst->fptr);
  int res = static_assert_declaration(fInst, typeMap);
  if (!res) {
    Declaration* decl = malloc(sizeof(Declaration));
    initDeclaration(decl);

    // Declaration Specifiers
    fseek(fInst->fptr, fpos, SEEK_SET);
    if ((res = declaration_specifiers(fInst, typeMap, &decl) &&
               (init_declarator_list(fInst, typeMap, decl) || 1) &&
               expectToken(fInst, Tok_Punct, Punct_semi))) {
      Identifier* ident = malloc(sizeof(Identifier));
      ident->identType = Ident_Value;
      ident->declaration = decl;
      listAdd(declList, ident);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int declaration_specifiers(FileInst* fInst,
                           Map* typeMap,
                           Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  Declaration* decl = *declPtr;
  int res = storage_class_specifier(fInst, typeMap, *declPtr) &&
            (declaration_specifiers(fInst, typeMap, declPtr) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    // TODO:
    res = type_specifier(fInst, typeMap, declPtr) &&
          (declaration_specifiers(fInst, typeMap, declPtr) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_qualifier(fInst, typeMap, &(decl->qualifier)) &&
          (declaration_specifiers(fInst, typeMap, declPtr) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = function_specifier(fInst, typeMap) &&
          (declaration_specifiers(fInst, typeMap, declPtr) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = alignment_specifier(fInst, typeMap) &&
          (declaration_specifiers(fInst, typeMap, declPtr) || 1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int init_declarator_list(FileInst* fInst, Map* typeMap, Declaration* decl) {
  long int fpos = ftell(fInst->fptr);
  int res = init_declarator(fInst, typeMap, decl) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              init_declarator_list(fInst, typeMap, decl)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int init_declarator(FileInst* fInst, Map* typeMap, Declaration* decl) {
  long int fpos = ftell(fInst->fptr);
  int res = declarator(fInst, typeMap, decl) &&
            ((expectToken(fInst, Tok_Punct, Punct_assign) &&
              initializer(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int storage_class_specifier(FileInst* fInst, Map* typeMap, Declaration* decl) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_typedef);
  char storage = (res) ? Type_Typedef : 0;
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_extern);
    storage = (res) ? Type_Extern : 0;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_static);
    storage = (res) ? Type_Static : 0;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Thread_local);
    storage = (res) ? Type_Thread_local : 0;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_auto);
    storage = (res) ? Type_Auto : 0;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_register);
    storage = (res) ? Type_Register : 0;
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    // Sementic: At most, one storage-class specifier may be given in the
    // declaration specifiers in a declaration
    if (decl->storage & 7) {
      fprintf(stderr, WVMCC_ERR_TOO_MANY_STORAGE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->storage = storage | (decl->storage & 8);
    // Sementic: _Thread_local may appear with static or extern
    if ((decl->storage > 8) && decl->storage != 10 && decl->storage != 11) {
      fprintf(stderr, WVMCC_ERR_INVALID_THREAD_LOCAL_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    return 1;
  }
}

int type_specifier(FileInst* fInst, Map* typeMap, Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  Declaration* decl = *declPtr;
  int res = expectToken(fInst, Tok_Keyword, Keyw_long);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_int);
  } else {
    // long
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_LONG_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    if (decl->qualifier & Type_Longlong) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_TOO_MORE_LONG_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->qualifier += Type_Long;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_short);
  } else {
    // int
    if (decl->specifier && decl->specifier != Type_Short) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->specifier = (decl->specifier != Type_Short) ? Type_Int : Type_Short;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_char);
  } else {
    // short
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->specifier = Type_Short;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_void);
  } else {
    // char
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->specifier = Type_Char;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_float);
  } else {
    // void
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->specifier = Type_Void;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_double);
  } else {
    // float
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->specifier = Type_float;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_signed);
  } else {
    // double
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    decl->specifier = Type_float;
    decl->qualifier |= Type_Long;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_unsigned);
  } else {
    // signed
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    decl->qualifier &= ~Type_Unsigned;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Bool);
  } else {
    // unsigned
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    decl->qualifier |= Type_Unsigned;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Complex);
  } else {
    // _Bool
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    decl->specifier = Type_Bool;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = atomic_type_specifier(fInst, typeMap);
  } else {
    // _Complex
    if (decl->specifier) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    decl->specifier = Type_Complex;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = struct_or_union_specifier(fInst, typeMap, declPtr);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = enum_specifier(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = typedef_name(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_or_union_specifier(FileInst* fInst,
                              Map* typeMap,
                              Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  Declaration* decl = *declPtr;
  int res = struct_or_union(fInst, typeMap, decl);
  if (res) {
    StructUnion* newDeclaration = malloc(sizeof(StructUnion));
    newDeclaration->decl.qualifier = decl->qualifier;
    newDeclaration->decl.specifier = decl->specifier;
    newDeclaration->decl.storage = decl->storage;
    newDeclaration->props = listNew();
    freeDeclaration(decl);
    *declPtr = (Declaration*)newDeclaration;
    decl = *declPtr;
    Token* identifier = (Token*)expectToken(fInst, Tok_Ident, 0);
    if (identifier != NULL) {
      // TODO:
      res = (expectToken(fInst, Tok_Punct, Punct_braceL) &&
             struct_declaration_list(fInst, typeMap, declPtr) &&
             expectToken(fInst, Tok_Punct, Punct_braceR)) ||
            1;
    } else {
      // TODO:
      res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
            struct_declaration_list(fInst, typeMap, declPtr) &&
            expectToken(fInst, Tok_Punct, Punct_braceR);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_or_union(FileInst* fInst, Map* typeMap, Declaration* decl) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_struct);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_union);
    if (!res) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      return 0;
    }
    // Union
    decl->specifier = Type_Union;
  } else {
    // Struct
    decl->specifier = Type_Struct;
  }
  return 1;
}

int struct_declaration_list(FileInst* fInst,
                            Map* typeMap,
                            Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_declaration(fInst, typeMap, declPtr) &&
            (struct_declaration_list(fInst, typeMap, declPtr) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declaration(FileInst* fInst, Map* typeMap, Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  int res = static_assert_declaration(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = specifier_qualifier_list(fInst, typeMap, declPtr) &&
          (struct_declarator_list(fInst, typeMap, declPtr) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_semi);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int specifier_qualifier_list(FileInst* fInst,
                             Map* typeMap,
                             Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  Declaration* decl = *declPtr;
  int res = type_specifier(fInst, typeMap, declPtr) &&
            (specifier_qualifier_list(fInst, typeMap, declPtr) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_qualifier(fInst, typeMap, &(decl->qualifier)) &&
          (specifier_qualifier_list(fInst, typeMap, declPtr) || 1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declarator_list(FileInst* fInst,
                           Map* typeMap,
                           Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_declarator(fInst, typeMap, declPtr) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              struct_declarator_list(fInst, typeMap, declPtr)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declarator(FileInst* fInst, Map* typeMap, Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  int res = declarator(fInst, typeMap, *declPtr);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_colon) &&
          constant_expression(fInst, typeMap);
  } else {
    res = ((expectToken(fInst, Tok_Punct, Punct_colon) &&
            constant_expression(fInst, typeMap)) ||
           1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int enum_specifier(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_enum);
  if (res) {
    Token* identifier = (Token*)expectToken(fInst, Tok_Ident, 0);
    if (identifier == NULL) {
      res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
            enumerator_list(fInst, typeMap) &&
            (expectToken(fInst, Tok_Punct, Punct_comma) || 1) &&
            expectToken(fInst, Tok_Punct, Punct_braceR);
    } else {
      res = ((expectToken(fInst, Tok_Punct, Punct_braceL) &&
              enumerator_list(fInst, typeMap) &&
              (expectToken(fInst, Tok_Punct, Punct_comma) || 1) &&
              expectToken(fInst, Tok_Punct, Punct_braceR)) ||
             1);
    }
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int enumerator_list(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = enumerator(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              enumerator_list(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int enumerator(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  Token* enumeration_constant = (Token*)expectToken(fInst, Tok_Ident, 0);
  int res =
      enumeration_constant && ((expectToken(fInst, Tok_Punct, Punct_assign) &&
                                constant_expression(fInst, typeMap)) ||
                               1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int atomic_type_specifier(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_Atomic) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) &&
            type_name(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int type_qualifier(FileInst* fInst, Map* typeMap, char* qualifier) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_const);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_restrict);
  } else {
    // const
    *qualifier |= Type_Const;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_volatile);
  } else {
    // restrict
    *qualifier |= Type_Restrict;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Atomic);
  } else {
    // volatile
    *qualifier |= Type_Volatile;
    return 1;
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    // atomic
    *qualifier |= Type_Atomic;
    return 1;
  }
}

int function_specifier(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_inline);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Noreturn);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int alignment_specifier(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res =
      expectToken(fInst, Tok_Keyword, Keyw_Alignas) &&
      expectToken(fInst, Tok_Punct, Punct_paranL) &&
      (type_name(fInst, typeMap) || constant_expression(fInst, typeMap)) &&
      expectToken(fInst, Tok_Punct, Punct_paranR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int declarator(FileInst* fInst, Map* typeMap, Declaration* decl) {
  long int fpos = ftell(fInst->fptr);
  List *ptrList = listNew();
  int res = (pointer(fInst, typeMap, ptrList) || 1) &&
            direct_declarator(fInst, typeMap, decl);
  for(int i = 0; i < ptrList->size; ++i){
    if(res){
      listAdd(decl->declarators, listAt(ptrList, i));
    }else{
      free(listAt(ptrList, i));
    }
  }
  listFree(&ptrList);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

// Used to elimate left-recursion
static int direct_declarator_tail(FileInst* fInst,
                                  Map* typeMap,
                                  Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            (parameter_type_list(fInst, typeMap, declPtr) ||
             (identifier_list(fInst, typeMap) || 1)) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    ArrayDeclarator* arrDecl = malloc(sizeof(ArrayDeclarator));
    arrDecl->declType = Decl_Array;
    arrDecl->qualifier = 0;
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst, typeMap, &(arrDecl->qualifier)) || 1) &&
          (assignment_expression(fInst, typeMap) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
    if(res){
      listAdd((*declPtr)->declarators, arrDecl);
    }else{
      free(arrDecl);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    ArrayDeclarator* arrDecl = malloc(sizeof(ArrayDeclarator));
    arrDecl->declType = Decl_Array;
    arrDecl->qualifier = 0;
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          (type_qualifier_list(fInst, typeMap, &(arrDecl->qualifier)) || 1) &&
          assignment_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
    arrDecl->qualifier |= Array_Static;
    if(res){
      listAdd((*declPtr)->declarators, arrDecl);
    }else{
      free(arrDecl);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    ArrayDeclarator* arrDecl = malloc(sizeof(ArrayDeclarator));
    arrDecl->declType = Decl_Array;
    arrDecl->qualifier = 0;
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          type_qualifier_list(fInst, typeMap, &(arrDecl->qualifier)) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          assignment_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
    arrDecl->qualifier |= Array_Static;
    if(res){
      listAdd((*declPtr)->declarators, arrDecl);
    }else{
      free(arrDecl);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    ArrayDeclarator* arrDecl = malloc(sizeof(ArrayDeclarator));
    arrDecl->declType = Decl_Array;
    arrDecl->qualifier = 0;
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst, typeMap, &(arrDecl->qualifier)) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_aster) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
    arrDecl->qualifier |= Array_Unspecified;
    if(res){
      listAdd((*declPtr)->declarators, arrDecl);
    }else{
      free(arrDecl);
    }
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    direct_declarator_tail(fInst, typeMap, declPtr);
    return 1;
  }
}
int direct_declarator(FileInst* fInst, Map* typeMap, Declaration* decl) {
  long int fpos = ftell(fInst->fptr);
  Token* identifier = (Token*)expectToken(fInst, Tok_Ident, 0);
  if (identifier) {
    decl->identifier = identifier->data.str;
  } else {
    int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
              declarator(fInst, typeMap, decl) &&
              expectToken(fInst, Tok_Punct, Punct_paranR);
    if (!res) {
      fseek(fInst->fptr, fpos, SEEK_SET);
      return 0;
    }
  }
  direct_declarator_tail(fInst, typeMap, &decl);
  return 1;
}

int pointer(FileInst* fInst, Map* typeMap, List* ptrList) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_aster);
  if (res) {
    char qualifier = 0;
    type_qualifier_list(fInst, typeMap, &qualifier);
    PointerDeclarator *ptrDecl = malloc(sizeof(PointerDeclarator));
    ptrDecl->declType = Decl_Pointer;
    ptrDecl->qualifier = qualifier;
    listAdd(ptrList, ptrDecl);
    pointer(fInst, typeMap, ptrList);
    return 1;
  } else {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  }
}

int type_qualifier_list(FileInst* fInst, Map* typeMap, char* qualifier) {
  long int fpos = ftell(fInst->fptr);
  int res = type_qualifier(fInst, typeMap, qualifier) &&
            (type_qualifier_list(fInst, typeMap, qualifier) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int parameter_type_list(FileInst* fInst, Map* typeMap, Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  ParamDeclarator *paramDecl = malloc(sizeof(ParamDeclarator));
  paramDecl->params = listNew();
  paramDecl->declType = Decl_Function;
  int res = parameter_list(fInst, typeMap, paramDecl);
  if(res){
    if(expectToken(fInst, Tok_Punct, Punct_comma) && expectToken(fInst, Tok_Punct, Punct_ellips)){
      paramDecl->declType = Decl_Function_va;
    }
    listAdd((*declPtr)->declarators, paramDecl);
    return 1;
  } else {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  }
}

int parameter_list(FileInst* fInst, Map* typeMap, ParamDeclarator* paramDecl) {
  long int fpos = ftell(fInst->fptr);
  Declaration *decl = malloc(sizeof(Declaration));
  initDeclaration(decl);
  int res = parameter_declaration(fInst, typeMap, &decl) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              parameter_list(fInst, typeMap, paramDecl)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    listAdd(paramDecl->params, decl);
    return 1;
  }
}

int parameter_declaration(FileInst* fInst,
                          Map* typeMap,
                          Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  int res = declaration_specifiers(fInst, typeMap, declPtr) &&
            (declarator(fInst, typeMap, *declPtr) ||
             (abstract_declarator(fInst, typeMap, declPtr) || 1));

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int identifier_list(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  Token* identifier = (Token*)expectToken(fInst, Tok_Ident, 0);
  int res = identifier && ((expectToken(fInst, Tok_Punct, Punct_comma) &&
                            identifier_list(fInst, typeMap)) ||
                           1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int type_name(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  Declaration* decl = malloc(sizeof(Declaration));
  int res = specifier_qualifier_list(fInst, typeMap, &decl) &&
            (abstract_declarator(fInst, typeMap, &decl) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int abstract_declarator(FileInst* fInst, Map* typeMap, Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  List *ptrList = listNew();
  int res = direct_abstract_declarator(fInst, typeMap, declPtr) ||
            (pointer(fInst, typeMap, ptrList) &&
             (direct_abstract_declarator(fInst, typeMap, declPtr) || 1));

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

// Used to elimate left-recursion
static int direct_abstract_declarator_tail(FileInst* fInst,
                                           Map* typeMap,
                                           Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  Declaration* decl = *declPtr;
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            (parameter_type_list(fInst, typeMap, declPtr) || 1) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst, typeMap, &(decl->qualifier)) || 1) &&
          (assignment_expression(fInst, typeMap) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          (type_qualifier_list(fInst, typeMap, &(decl->qualifier)) || 1) &&
          assignment_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          type_qualifier_list(fInst, typeMap, &(decl->qualifier)) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          assignment_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          expectToken(fInst, Tok_Punct, Punct_aster) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int direct_abstract_declarator(FileInst* fInst,
                               Map* typeMap,
                               Declaration** declPtr) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            abstract_declarator(fInst, typeMap, declPtr) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) &&
            direct_abstract_declarator_tail(fInst, typeMap, declPtr);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int typedef_name(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  Token* identifier = (Token*)expectToken(fInst, Tok_Ident, 0);
  Declaration* type = NULL;
  if (identifier == NULL ||
      mapGet(typeMap, identifier->data.str, (void**)&type) == -1) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int initializer(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = assignment_expression(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
          initializer_list(fInst, typeMap) &&
          (expectToken(fInst, Tok_Punct, Punct_comma) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_braceR);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int initializer_list(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = (designation(fInst, typeMap) || 1) && initializer(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = initializer_list(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_comma) &&
          (designation(fInst, typeMap) || 1) && initializer(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int designation(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = designator_list(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_assign);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int designator_list(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = designtor(fInst, typeMap) && (designator_list(fInst, typeMap) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int designtor(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
            constant_expression(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_brackR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    Token* identifier = NULL;
    res = expectToken(fInst, Tok_Punct, Punct_dot) &&
          (identifier = (Token*)expectToken(fInst, Tok_Ident, 0));
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int static_assert_declaration(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  Token* stringLiteral = NULL;
  int res = expectToken(fInst, Tok_Keyword, Keyw_Static_assert) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) &&
            constant_expression(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_comma) &&
            (stringLiteral = (Token*)expectToken(fInst, Tok_String, 0)) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) &&
            expectToken(fInst, Tok_Punct, Punct_semi);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}