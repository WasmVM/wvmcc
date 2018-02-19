#include "rules.h"

int declaration(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = static_assert_declaration(fInst, typeMap);
  if (!res) {
    Type *declType = malloc(sizeof(Type));
    initType(declType);
    // Declaration Specifiers
    fseek(fInst->fptr, fpos, SEEK_SET);
    // TODO:
    res = declaration_specifiers(fInst, typeMap, &declType) &&
          (init_declarator_list(fInst, typeMap) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_semi);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int declaration_specifiers(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  // TODO:
  long int fpos = ftell(fInst->fptr);
  int res = storage_class_specifier(fInst, typeMap, *declTypePtr) &&
            (declaration_specifiers(fInst, typeMap, declTypePtr) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_specifier(fInst, typeMap, declTypePtr) &&
          (declaration_specifiers(fInst, typeMap, declTypePtr) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_qualifier(fInst, typeMap) &&
          (declaration_specifiers(fInst, typeMap, declTypePtr) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = function_specifier(fInst, typeMap) &&
          (declaration_specifiers(fInst, typeMap, declTypePtr) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = alignment_specifier(fInst, typeMap) &&
          (declaration_specifiers(fInst, typeMap, declTypePtr) || 1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int init_declarator_list(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = init_declarator(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              init_declarator_list(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int init_declarator(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = declarator(fInst, typeMap) &&
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

int storage_class_specifier(FileInst* fInst, Map* typeMap, Type* declType) {
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
    if (declType->storage & 7) {
      fprintf(stderr, WVMCC_ERR_TOO_MANY_STORAGE_SPECIFIER, getShortName(fInst),
              fInst->curline);
      return 0;
    }
    declType->storage = storage | (declType->storage & 8);
    // Sementic: _Thread_local may appear with static or extern
    if ((declType->storage > 8) && declType->storage != 10 &&
        declType->storage != 11) {
      fprintf(stderr, WVMCC_ERR_INVALID_THREAD_LOCAL_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    return 1;
  }
}

int type_specifier(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  Type *declType = *declTypePtr;
  int res = expectToken(fInst, Tok_Keyword, Keyw_long);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_int);
  }else{
    // long
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_LONG_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    if(declType->optional & Type_Longlong){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_TOO_MORE_LONG_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->optional += Type_Long;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_short);
  }else{
    // int
    if(declType->sprcifier && declType->sprcifier != Type_Short){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = (declType->sprcifier != Type_Short) ? Type_Int : Type_Short;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_char);
  }else{
    // short
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = Type_Short;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_void);
  }else{
    // char
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = Type_Char;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_float);
  }else{
    // void
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = Type_Void;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_double);
  }else{
    // float
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = Type_float;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_signed);
  }else{
    // double
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_DUPLICATE_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = Type_float;
    declType->optional |= Type_Long;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_unsigned);
  }else{
    // signed
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->optional &= ~Type_Unsigned;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Bool);
  }else{
    // unsigned
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->optional |= Type_Unsigned;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Complex);
  }else{
    // _Bool
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = Type_Bool;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = atomic_type_specifier(fInst, typeMap);
  }else{
    // _Complex
    if(declType->sprcifier){
      fseek(fInst->fptr, fpos, SEEK_SET);
      fprintf(stderr, WVMCC_ERR_INVALID_SIGNED_UNSIGNED_SPECIFIER,
              getShortName(fInst), fInst->curline);
      return 0;
    }
    declType->sprcifier = Type_Complex;
    return 1;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = struct_or_union_specifier(fInst, typeMap, declTypePtr);
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

int struct_or_union_specifier(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_or_union(fInst, typeMap, *declTypePtr);
  if (res) {
    Token* identifier = (Token*)expectToken(fInst, Tok_Ident, 0);
    if (identifier != NULL) {
      res = (expectToken(fInst, Tok_Punct, Punct_braceL) &&
             struct_declaration_list(fInst, typeMap, declTypePtr) &&
             expectToken(fInst, Tok_Punct, Punct_braceR)) ||
            1;
    } else {
      res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
            struct_declaration_list(fInst, typeMap, declTypePtr) &&
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

int struct_or_union(FileInst* fInst, Map* typeMap, Type* declType) {
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
    declType->sprcifier = Type_Union;
  }else{
    // Struct
    declType->sprcifier = Type_Struct;
  }
  return 1;
}

int struct_declaration_list(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_declaration(fInst, typeMap, declTypePtr) &&
            (struct_declaration_list(fInst, typeMap, declTypePtr) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declaration(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = static_assert_declaration(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = specifier_qualifier_list(fInst, typeMap, declTypePtr) &&
          (struct_declarator_list(fInst, typeMap, declTypePtr) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_semi);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int specifier_qualifier_list(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = type_specifier(fInst, typeMap, declTypePtr) &&
            (specifier_qualifier_list(fInst, typeMap, declTypePtr) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_qualifier(fInst, typeMap) &&
          (specifier_qualifier_list(fInst, typeMap, declTypePtr) || 1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declarator_list(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_declarator(fInst, typeMap, declTypePtr) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              struct_declarator_list(fInst, typeMap, declTypePtr)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declarator(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = declarator(fInst, typeMap);
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

int type_qualifier(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_const);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_restrict);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_volatile);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Atomic);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
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

int declarator(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = (pointer(fInst, typeMap) || 1) && direct_declarator(fInst, typeMap);

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
                                  Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            (parameter_type_list(fInst, typeMap, declTypePtr) ||
             (identifier_list(fInst, typeMap) || 1)) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst, typeMap) || 1) &&
          (assignment_expression(fInst, typeMap) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          (type_qualifier_list(fInst, typeMap) || 1) &&
          assignment_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          type_qualifier_list(fInst, typeMap) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          assignment_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst, typeMap) || 1) &&
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
int direct_declarator(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  Token* identifier = (Token*)expectToken(fInst, Tok_Ident, 0);
  Type *declType = malloc(sizeof(Type));
  int res = (identifier || (expectToken(fInst, Tok_Punct, Punct_paranL) &&
                            declarator(fInst, typeMap) &&
                            expectToken(fInst, Tok_Punct, Punct_paranR))) &&
            direct_declarator_tail(fInst, typeMap, &declType);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int pointer(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_aster) &&
            (type_qualifier_list(fInst, typeMap) || 1) &&
            (pointer(fInst, typeMap) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int type_qualifier_list(FileInst* fInst, Map* typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = type_qualifier(fInst, typeMap) &&
            (type_qualifier_list(fInst, typeMap) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int parameter_type_list(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = parameter_list(fInst, typeMap, declTypePtr) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              expectToken(fInst, Tok_Punct, Punct_ellips)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int parameter_list(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = parameter_declaration(fInst, typeMap, declTypePtr) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              parameter_list(fInst, typeMap, declTypePtr)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int parameter_declaration(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = declaration_specifiers(fInst, typeMap, declTypePtr) &&
            (declarator(fInst, typeMap) ||
             (abstract_declarator(fInst, typeMap, declTypePtr) || 1));

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
  Type *declType = malloc(sizeof(Type));
  int res = specifier_qualifier_list(fInst, typeMap, &declType) &&
            (abstract_declarator(fInst, typeMap, &declType) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int abstract_declarator(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = direct_abstract_declarator(fInst, typeMap, declTypePtr) ||
            (pointer(fInst, typeMap) &&
             (direct_abstract_declarator(fInst, typeMap, declTypePtr) || 1));

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
                                           Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            (parameter_type_list(fInst, typeMap, declTypePtr) || 1) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst, typeMap) || 1) &&
          (assignment_expression(fInst, typeMap) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          (type_qualifier_list(fInst, typeMap) || 1) &&
          assignment_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          type_qualifier_list(fInst, typeMap) &&
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
int direct_abstract_declarator(FileInst* fInst, Map* typeMap, Type** declTypePtr) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            abstract_declarator(fInst, typeMap, declTypePtr) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) &&
            direct_abstract_declarator_tail(fInst, typeMap, declTypePtr);

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
  Type* type = NULL;
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