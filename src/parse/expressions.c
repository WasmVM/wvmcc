#include "rules.h"

int primary_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  Token *token = (Token *)expectToken(fInst, Tok_Ident, 0);
  int res = (token != NULL);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Token *)expectToken(fInst, Tok_Int, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Token *)expectToken(fInst, Tok_Float, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Token *)expectToken(fInst, Tok_Char, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Token *)expectToken(fInst, Tok_String, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_paranR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = generic_selection(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int generic_selection(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_Generic) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) &&
            assignment_expression(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_comma) &&
            generic_assoc_list(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int generic_assoc_list(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = generic_association(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              generic_assoc_list(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int generic_association(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = type_name(fInst, typeMap) && expectToken(fInst, Tok_Punct, Punct_colon) &&
            assignment_expression(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_default) &&
          expectToken(fInst, Tok_Punct, Punct_colon) &&
          assignment_expression(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

static int postfix_expression_tail(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_brackL) && expression(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_brackR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
          (argument_expression_list(fInst, typeMap) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_paranR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    Token *identifier = NULL;
    res = expectToken(fInst, Tok_Punct, Punct_dot) &&
          (identifier = (Token *)expectToken(fInst, Tok_Ident, 0));
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    Token *identifier = NULL;
    res = expectToken(fInst, Tok_Punct, Punct_arrow) &&
          (identifier = (Token *)expectToken(fInst, Tok_Ident, 0));
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_inc);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_dec);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int postfix_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = primary_expression(fInst, typeMap) && postfix_expression_tail(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) && type_name(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_paranR) &&
          expectToken(fInst, Tok_Punct, Punct_braceL) &&
          initializer_list(fInst, typeMap) &&
          (expectToken(fInst, Tok_Punct, Punct_comma) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_braceR) &&
          postfix_expression_tail(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int argument_expression_list(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = assignment_expression(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              argument_expression_list(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int unary_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = postfix_expression(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_inc) && unary_expression(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_dec) && unary_expression(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = unary_operator(fInst, typeMap) && cast_expression(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res =
        expectToken(fInst, Tok_Keyword, Keyw_sizeof) && unary_expression(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_sizeof) &&
          expectToken(fInst, Tok_Punct, Punct_paranL) && type_name(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_paranR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_Alignof) &&
          expectToken(fInst, Tok_Punct, Punct_paranL) && type_name(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_paranR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int unary_operator(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_amp);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_aster);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_plus);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_minus);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_tilde);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_exclm);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int cast_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = unary_expression(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) && type_name(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_paranR) && cast_expression(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int multiplicative_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = cast_expression(fInst, typeMap);
  if (res) {
    long int fpos1 = ftell(fInst->fptr);
    res &= expectToken(fInst, Tok_Punct, Punct_aster) &&
           multiplicative_expression(fInst, typeMap);
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_slash) &&
            multiplicative_expression(fInst, typeMap);
    }
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_percent) &&
            multiplicative_expression(fInst, typeMap);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int additive_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = multiplicative_expression(fInst, typeMap);
  if (res) {
    long int fpos1 = ftell(fInst->fptr);
    res &=
        expectToken(fInst, Tok_Punct, Punct_plus) && additive_expression(fInst, typeMap);
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_minus) &&
            additive_expression(fInst, typeMap);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int shift_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = additive_expression(fInst, typeMap);
  if (res) {
    long int fpos1 = ftell(fInst->fptr);
    res &=
        expectToken(fInst, Tok_Punct, Punct_shiftL) && shift_expression(fInst, typeMap);
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_shiftR) &&
            shift_expression(fInst, typeMap);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int relational_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = shift_expression(fInst, typeMap);
  if (res) {
    long int fpos1 = ftell(fInst->fptr);
    res &=
        expectToken(fInst, Tok_Punct, Punct_lt) && relational_expression(fInst, typeMap);
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_gt) &&
            relational_expression(fInst, typeMap);
    }
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_le) &&
            relational_expression(fInst, typeMap);
    }
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_ge) &&
            relational_expression(fInst, typeMap);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int equality_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = relational_expression(fInst, typeMap);
  if (res) {
    long int fpos1 = ftell(fInst->fptr);
    res &=
        expectToken(fInst, Tok_Punct, Punct_eq) && equality_expression(fInst, typeMap);
    if (!res) {
      fseek(fInst->fptr, fpos1, SEEK_SET);
      res = expectToken(fInst, Tok_Punct, Punct_neq) &&
            equality_expression(fInst, typeMap);
    }
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int and_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res =
      equality_expression(fInst, typeMap) &&
      ((expectToken(fInst, Tok_Punct, Punct_aster) && and_expression(fInst, typeMap)) ||
       1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int exclusive_or_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res =
      and_expression(fInst, typeMap) && ((expectToken(fInst, Tok_Punct, Punct_caret) &&
                                 exclusive_or_expression(fInst, typeMap)) ||
                                1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int inclusive_or_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = exclusive_or_expression(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_vbar) &&
              inclusive_or_expression(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int logical_and_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = inclusive_or_expression(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_and) &&
              logical_and_expression(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int logical_or_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = logical_and_expression(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_or) &&
              logical_or_expression(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int conditional_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = logical_or_expression(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_ques) && expression(fInst, typeMap) &&
              expectToken(fInst, Tok_Punct, Punct_colon) &&
              conditional_expression(fInst, typeMap)) ||
             1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int assignment_expression(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res =
      conditional_expression(fInst, typeMap) ||
      (unary_expression(fInst, typeMap) && expectToken(fInst, Tok_Punct, Punct_colon) &&
       conditional_expression(fInst, typeMap));
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int assignment_operator(FileInst *fInst, Map *typeMap){
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_assign);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_mult);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_div);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_mod);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_plus);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_minus);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_shl);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_shr);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_and);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_xor);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_ass_or);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int expression(FileInst *fInst, Map *typeMap){
  long int fpos = ftell(fInst->fptr);
  int res = assignment_expression(fInst, typeMap) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              expression(fInst, typeMap)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int constant_expression(FileInst *fInst, Map *typeMap){
  long int fpos = ftell(fInst->fptr);
  int res = conditional_expression(fInst, typeMap);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}