#include "rules.h"

int declaration(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = static_assert_declaration(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = declaration_specifiers(fInst) && (init_declarator_list(fInst) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_semi);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int declaration_specifiers(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      storage_class_specifier(fInst) && (declaration_specifiers(fInst) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_specifier(fInst) && (declaration_specifiers(fInst) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_qualifier(fInst) && (declaration_specifiers(fInst) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = function_specifier(fInst) && (declaration_specifiers(fInst) || 1);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = alignment_specifier(fInst) && (declaration_specifiers(fInst) || 1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int init_declarator_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      init_declarator(fInst) && ((expectToken(fInst, Tok_Punct, Punct_comma) &&
                                  init_declarator_list(fInst)) ||
                                 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int init_declarator(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      declarator(fInst) &&
      ((expectToken(fInst, Tok_Punct, Punct_assign) && initializer(fInst)) ||
       1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int storage_class_specifier(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_typedef);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_extern);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_static);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Thread_local);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_auto);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_register);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int type_specifier(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_void);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_char);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_short);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_int);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_long);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_float);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_double);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_signed);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_unsigned);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Bool);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Complex);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = atomic_type_specifier(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = struct_or_union_specifier(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = enum_specifier(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = typedef_name(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_or_union_specifier(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_or_union(fInst);
  if (res) {
    Node* identifier = (Node*)expectToken(fInst, Tok_Ident, 0);
    if (identifier != NULL) {
      res = (expectToken(fInst, Tok_Punct, Punct_braceL) &&
             struct_declaration_list(fInst) &&
             expectToken(fInst, Tok_Punct, Punct_braceR)) ||
            1;
    } else {
      res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
            struct_declaration_list(fInst) &&
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

int struct_or_union(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_struct);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_union);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declaration_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_declaration(fInst) && (struct_declaration_list(fInst) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declaration(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = static_assert_declaration(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = specifier_qualifier_list(fInst) &&
          (struct_declarator_list(fInst) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_semi);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int specifier_qualifier_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = type_specifier(fInst) && (specifier_qualifier_list(fInst) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = type_qualifier(fInst) && (specifier_qualifier_list(fInst) || 1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declarator_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = struct_declarator(fInst) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              struct_declarator_list(fInst)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int struct_declarator(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = declarator(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_colon) &&
          constant_expression(fInst);
  } else {
    res = ((expectToken(fInst, Tok_Punct, Punct_colon) &&
            constant_expression(fInst)) ||
           1);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int enum_specifier(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_enum);
  if (res) {
    Node* identifier = (Node*)expectToken(fInst, Tok_Ident, 0);
    if (identifier == NULL) {
      res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
            enumerator_list(fInst) &&
            (expectToken(fInst, Tok_Punct, Punct_comma) || 1) &&
            expectToken(fInst, Tok_Punct, Punct_braceR);
    } else {
      res = ((expectToken(fInst, Tok_Punct, Punct_braceL) &&
              enumerator_list(fInst) &&
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

int enumerator_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      enumerator(fInst) &&
      ((expectToken(fInst, Tok_Punct, Punct_comma) && enumerator_list(fInst)) ||
       1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int enumerator(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  Node* enumeration_constant = (Node*)expectToken(fInst, Tok_Ident, 0);
  int res =
      enumeration_constant && ((expectToken(fInst, Tok_Punct, Punct_assign) &&
                                constant_expression(fInst)) ||
                               1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int atomic_type_specifier(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_Atomic) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) && type_name(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int type_qualifier(FileInst* fInst) {
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

int function_specifier(FileInst* fInst) {
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

int alignment_specifier(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_Alignas) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) &&
            (type_name(fInst) || constant_expression(fInst)) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int declarator(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = (pointer(fInst) || 1) && direct_declarator(fInst);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

// Used to elimate left-recursion
static int direct_declarator_tail(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            (parameter_type_list(fInst) || (identifier_list(fInst) || 1)) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst) || 1) &&
          (assignment_expression(fInst) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          (type_qualifier_list(fInst) || 1) && assignment_expression(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          type_qualifier_list(fInst) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          assignment_expression(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst) || 1) &&
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
int direct_declarator(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  Node* identifier = (Node*)expectToken(fInst, Tok_Ident, 0);
  int res = (identifier || (expectToken(fInst, Tok_Punct, Punct_paranL) &&
                            declarator(fInst) &&
                            expectToken(fInst, Tok_Punct, Punct_paranR))) &&
            direct_declarator_tail(fInst);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int pointer(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_aster) &&
            (type_qualifier_list(fInst) || 1) && (pointer(fInst) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int type_qualifier_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = type_qualifier(fInst) && (type_qualifier_list(fInst) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int parameter_type_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      parameter_list(fInst) && ((expectToken(fInst, Tok_Punct, Punct_comma) &&
                                 expectToken(fInst, Tok_Punct, Punct_ellips)) ||
                                1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int parameter_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      parameter_declaration(fInst) &&
      ((expectToken(fInst, Tok_Punct, Punct_comma) && parameter_list(fInst)) ||
       1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int parameter_declaration(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = declaration_specifiers(fInst) &&
            (declarator(fInst) || (abstract_declarator(fInst) || 1));

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int identifier_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  Node* identifier = (Node*)expectToken(fInst, Tok_Ident, 0);
  int res =
      identifier &&
      ((expectToken(fInst, Tok_Punct, Punct_comma) && identifier_list(fInst)) ||
       1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int type_name(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      specifier_qualifier_list(fInst) && (abstract_declarator(fInst) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int abstract_declarator(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = direct_abstract_declarator(fInst) ||
            (pointer(fInst) && (direct_abstract_declarator(fInst) || 1));

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

// Used to elimate left-recursion
static int direct_abstract_declarator_tail(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            (parameter_type_list(fInst) || 1) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          (type_qualifier_list(fInst) || 1) &&
          (assignment_expression(fInst) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          (type_qualifier_list(fInst) || 1) && assignment_expression(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_brackR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
          type_qualifier_list(fInst) &&
          expectToken(fInst, Tok_Keyword, Keyw_static) &&
          assignment_expression(fInst) &&
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
int direct_abstract_declarator(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
            abstract_declarator(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) &&
            direct_abstract_declarator_tail(fInst);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int typedef_name(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  Node* identifier = (Node*)expectToken(fInst, Tok_Ident, 0);

  if (identifier == NULL) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int initializer(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = assignment_expression(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
          initializer_list(fInst) &&
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

int initializer_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = (designation(fInst) || 1) && initializer(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = initializer_list(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_comma) &&
          (designation(fInst) || 1) && initializer(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int designation(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      designator_list(fInst) && expectToken(fInst, Tok_Punct, Punct_assign);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int designator_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = designtor(fInst) && (designator_list(fInst) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int designtor(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_brackL) &&
            constant_expression(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_brackR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    Node* identifier = NULL;
    res = expectToken(fInst, Tok_Punct, Punct_dot) &&
          (identifier = (Node*)expectToken(fInst, Tok_Ident, 0));
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int static_assert_declaration(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  Node* stringLiteral = NULL;
  int res = expectToken(fInst, Tok_Keyword, Keyw_Static_assert) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) &&
            constant_expression(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_comma) &&
            (stringLiteral = (Node*)expectToken(fInst, Tok_String, 0)) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) &&
            expectToken(fInst, Tok_Punct, Punct_semi);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}