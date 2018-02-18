#include "rules.h"

static int identComp(void *a, void *b){
  return strcmp(a, b);
}

int startParse(FileInst* fInst, FILE* fout) {
  Map *typeMap = mapNew(identComp, free, free);
  translation_unit(fInst, typeMap);
  Token* token = getToken(fInst);
  if (token == NULL || token->type != Tok_EOF) {
    mapFree(&typeMap);
    return -1;
  }
  mapFree(&typeMap);
  return 0;
}

int translation_unit(FileInst* fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  /* external_declaration translation_unit?
   */
  int res = external_declaration(fInst, typeMap) && (translation_unit(fInst, typeMap) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int external_declaration(FileInst* fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  /* function_declaration
   * declaration
   */
  int res = preprocessor_hint(fInst) || function_definition(fInst, typeMap) ||
            declaration(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int function_definition(FileInst* fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  /* declaration-specifiers declarator declaration-list? compound-statement
   */
  int res = declaration_specifiers(fInst, typeMap) && declarator(fInst, typeMap) &&
            (declaration_list(fInst, typeMap) || 1) && compound_statement(fInst, typeMap);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int declaration_list(FileInst* fInst, Map *typeMap) {
  long int fpos = ftell(fInst->fptr);
  int res = declaration(fInst, typeMap) && (declaration_list(fInst, typeMap) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int preprocessor_hint(FileInst* fInst) {
  long int fpos = ftell(fInst->fptr);
  Token *lineNum = NULL, *filePath = NULL, *flag = NULL;
  int res = expectToken(fInst, Tok_Punct, Punct_hash);
  if (res) {
    Token* lineNum = (Token*)expectToken(fInst, Tok_Int, 0);
    Token* filePath = (Token*)expectToken(fInst, Tok_String, 0);
    Token* flag = (Token*)expectToken(fInst, Tok_Int, 0);
    if (lineNum && filePath && flag) {
      fInst->curline = lineNum->data.intVal;
      free(fInst->fname);
      fInst->fname = filePath->data.str;
    }
    free(lineNum);
    free(filePath);
    free(flag);
  }
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}