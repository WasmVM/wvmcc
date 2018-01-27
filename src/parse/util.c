#include "token.h"

intptr_t expectToken(FileInst* fileInst, TokenType type, int value) {
  long int fpos = ftell(fileInst->fptr);
  Token *token = getToken(fileInst);
  if (token != NULL && token->type == type) {
    if (value && (type == Tok_Punct || type == Tok_Keyword)){
      int val = (type == Tok_Punct) ? token->data.punct : token->data.keyword;
      free(token);
      if(val != value){
        fseek(fileInst->fptr, fpos, SEEK_SET);
        return 0;
      }
      return 1;
    }
    return (intptr_t) token;
  }
  free(token);
  fseek(fileInst->fptr, fpos, SEEK_SET);
  return (intptr_t) NULL;
}