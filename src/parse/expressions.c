#include "rules.h"

int primary_expression(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  Node* token = (Node*)expectToken(fInst, Tok_Ident, 0);
  int res = token != NULL;
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Node*)expectToken(fInst, Tok_Int, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Node*)expectToken(fInst, Tok_Float, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Node*)expectToken(fInst, Tok_Char, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    token = (Node*)expectToken(fInst, Tok_String, 0);
    res = token != NULL;
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_paranR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = generic_selection(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int generic_selection(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_Generic) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) &&
            assignment_expression(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_comma) &&
            generic_assoc_list(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_paranR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int generic_assoc_list(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = generic_association(fInst) &&
            ((expectToken(fInst, Tok_Punct, Punct_comma) &&
              generic_assoc_list(fInst)) ||
             1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int generic_association(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = type_name(fInst) && expectToken(fInst, Tok_Punct, Punct_colon) &&
            assignment_expression(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_default) &&
          expectToken(fInst, Tok_Punct, Punct_colon) &&
          assignment_expression(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

static int postfix_expression_tail(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_brackL) && expression(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_brackR);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
          (argument_expression_list(fInst) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_paranR);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    Node* identifier = NULL;
    res = expectToken(fInst, Tok_Punct, Punct_dot) &&
          (identifier = (Node*)expectToken(fInst, Tok_Ident, 0));
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) &&
          (argument_expression_list(fInst) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_paranR);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int postfix_expression(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = primary_expression(fInst) && postfix_expression_tail(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Punct, Punct_paranL) && type_name(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_paranR) &&
          expectToken(fInst, Tok_Punct, Punct_braceL) &&
          initializer_list(fInst) &&
          (expectToken(fInst, Tok_Punct, Punct_comma) || 1) &&
          expectToken(fInst, Tok_Punct, Punct_braceR) &&
          postfix_expression_tail(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int argument_expression_list(FileInst* fInst);
int unary_expression(FileInst* fInst);
int unary_operator(FileInst* fInst);
int cast_expression(FileInst* fInst);
int multiplicative_expression(FileInst* fInst);
int additive_expression(FileInst* fInst);
int shift_expression(FileInst* fInst);
int relational_expression(FileInst* fInst);
int equality_expression(FileInst* fInst);
int and_expression(FileInst* fInst);
int exclusive_or_expression(FileInst* fInst);
int inclusive_or_expression(FileInst* fInst);
int logical_and_expression(FileInst* fInst);
int logical_or_expression(FileInst* fInst);
int conditional_expression(FileInst* fInst);
int assignment_expression(FileInst* fInst);
int assignment_operator(FileInst* fInst);
int expression(FileInst* fInst);
int constant_expression(FileInst* fInst);