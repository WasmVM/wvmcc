#include "rules.h"

int statement(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = labeled_statement(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = compound_statement(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expression_statement(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = selection_statement(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = iteration_statement(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = jump_statement(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int labeled_statement(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  Token *identifier = (Token *)expectToken(fInst, Tok_Ident, 0);
  int res = identifier && expectToken(fInst, Tok_Punct, Punct_colon) &&
            statement(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_case) &&
          constant_expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_colon) && statement(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_default) &&
          expectToken(fInst, Tok_Punct, Punct_colon) && statement(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int compound_statement(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
            (block_item_list(fInst, typeMap) || 1) &&
            expectToken(fInst, Tok_Punct, Punct_braceR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int block_item_list(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = block_item(fInst, typeMap) && (block_item_list(fInst, typeMap) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int block_item(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = declaration(fInst, typeMap) || statement(fInst, typeMap);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int expression_statement(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res =
      (expression(fInst, typeMap) || 1) && expectToken(fInst, Tok_Punct, Punct_semi);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int selection_statement(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_switch) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_if) &&
          expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst, typeMap);
    if (res) {
      res = expectToken(fInst, Tok_Keyword, Keyw_else) && statement(fInst, typeMap);
    }
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int iteration_statement(FileInst *fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_while) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst, typeMap) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_do) && statement(fInst, typeMap) &&
          expectToken(fInst, Tok_Keyword, Keyw_while) &&
          expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst, typeMap) &&
          expectToken(fInst, Tok_Punct, Punct_paranR) &&
          expectToken(fInst, Tok_Punct, Punct_semi);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res =
        expectToken(fInst, Tok_Keyword, Keyw_for) &&
        expectToken(fInst, Tok_Punct, Punct_paranL) &&
        declaration(fInst, typeMap) &&
        (expression(fInst, typeMap) || 1) && expectToken(fInst, Tok_Punct, Punct_semi) &&
        (expression(fInst, typeMap) || 1) &&
        expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst, typeMap);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res =
        expectToken(fInst, Tok_Keyword, Keyw_for) &&
        expectToken(fInst, Tok_Punct, Punct_paranL) &&
        (expression(fInst, typeMap) || 1) && expectToken(fInst, Tok_Punct, Punct_semi) &&
        (expression(fInst, typeMap) || 1) && expectToken(fInst, Tok_Punct, Punct_semi) &&
        (expression(fInst, typeMap) || 1) &&
        expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst, typeMap);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int jump_statement(FileInst *fInst, Map *typeMap){
      long int fpos = ftell(fInst->fptr);
      Token *identifier = NULL;
  int res = expectToken(fInst, Tok_Keyword, Keyw_goto) && (identifier = (Token *)expectToken(fInst, Tok_Ident, 0)) &&
            expectToken(fInst, Tok_Punct, Punct_semi);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_continue) && 
            expectToken(fInst, Tok_Punct, Punct_semi);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_break) && 
            expectToken(fInst, Tok_Punct, Punct_semi);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res =
        expectToken(fInst, Tok_Keyword, Keyw_return) &&
        (expression(fInst, typeMap) || 1) && expectToken(fInst, Tok_Punct, Punct_semi);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}