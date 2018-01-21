#include "rules.h"

int statement(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = labeled_statement(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = compound_statement(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expression_statement(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = selection_statement(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = iteration_statement(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = jump_statement(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int labeled_statement(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  Node *identifier = (Node *)expectToken(fInst, Tok_Ident, 0);
  int res = identifier && expectToken(fInst, Tok_Punct, Punct_colon) &&
            statement(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_case) &&
          constant_expression(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_colon) && statement(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_default) &&
          expectToken(fInst, Tok_Punct, Punct_colon) && statement(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int compound_statement(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Punct, Punct_braceL) &&
            (block_item_list(fInst) || 1) &&
            expectToken(fInst, Tok_Punct, Punct_braceR);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int block_item_list(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = block_item(fInst) && (block_item_list(fInst) || 1);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int block_item(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = declaration(fInst) || statement(fInst);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int expression_statement(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  int res =
      (expression(fInst) || 1) && expectToken(fInst, Tok_Punct, Punct_semi);

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int selection_statement(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_switch) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_if) &&
          expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst);
    if (res) {
      res = expectToken(fInst, Tok_Keyword, Keyw_else) && statement(fInst);
    }
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int iteration_statement(FileInst *fInst) {
  long int fpos = ftell(fInst->fptr);
  int res = expectToken(fInst, Tok_Keyword, Keyw_while) &&
            expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst) &&
            expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res = expectToken(fInst, Tok_Keyword, Keyw_do) && statement(fInst) &&
          expectToken(fInst, Tok_Keyword, Keyw_while) &&
          expectToken(fInst, Tok_Punct, Punct_paranL) && expression(fInst) &&
          expectToken(fInst, Tok_Punct, Punct_paranR) &&
          expectToken(fInst, Tok_Punct, Punct_semi);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res =
        expectToken(fInst, Tok_Keyword, Keyw_for) &&
        expectToken(fInst, Tok_Punct, Punct_paranL) &&
        declaration(fInst) &&
        (expression(fInst) || 1) && expectToken(fInst, Tok_Punct, Punct_semi) &&
        (expression(fInst) || 1) &&
        expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    res =
        expectToken(fInst, Tok_Keyword, Keyw_for) &&
        expectToken(fInst, Tok_Punct, Punct_paranL) &&
        (expression(fInst) || 1) && expectToken(fInst, Tok_Punct, Punct_semi) &&
        (expression(fInst) || 1) && expectToken(fInst, Tok_Punct, Punct_semi) &&
        (expression(fInst) || 1) &&
        expectToken(fInst, Tok_Punct, Punct_paranR) && statement(fInst);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}
int jump_statement(FileInst *fInst){
      long int fpos = ftell(fInst->fptr);
      Node *identifier = NULL;
  int res = expectToken(fInst, Tok_Keyword, Keyw_goto) && (identifier = (Node *)expectToken(fInst, Tok_Ident, 0)) &&
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
        (expression(fInst) || 1) && expectToken(fInst, Tok_Punct, Punct_semi);
  }

  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}