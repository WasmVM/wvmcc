#include "rules.h"

static int identComp(void *a, void *b){
  return strcmp(a, b);
}

int startParse(FileInst* fInst) {
  Map *typeMap = mapNew(identComp, free, free);
  List *declList = listNew();
  List *codeList = listNew();
  translation_unit(fInst, typeMap, declList, codeList);
  Token* token = getToken(fInst);
  if (token == NULL || token->type != Tok_EOF) {
    mapFree(&typeMap);
    return -1;
  }
  mapFree(&typeMap);
  return 0;
}

int translation_unit(FileInst* fInst, Map *typeMap, List *declList, List *codeList) {
  long int fpos = ftell(fInst->fptr);
  /* external_declaration translation_unit?
   */
  int res = external_declaration(fInst, typeMap, declList, codeList) && (translation_unit(fInst, typeMap, declList, codeList) || 1);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int external_declaration(FileInst* fInst, Map *typeMap, List *declList, List *codeList) {
  long int fpos = ftell(fInst->fptr);
  int res = preprocessor_hint(fInst);
  if(!res){
    if(function_definition(fInst, typeMap, declList)){

    }else if(declaration(fInst, typeMap, declList, codeList)){
      Identifier *ident = (Identifier *)declList->tail->data;
      ident->localidx = -1;
    }else{
      fseek(fInst->fptr, fpos, SEEK_SET);
      return 0;
    }
  }
  return 1;
}

int function_definition(FileInst* fInst, Map *typeMap, List *declList) {
  long int fpos = ftell(fInst->fptr);
  /* declaration-specifiers declarator declaration-list? compound-statement
   */
  Declaration *decl = malloc(sizeof(Declaration));
  initDeclaration(decl);
  List *codeList = listNew();
  int res = declaration_specifiers(fInst, typeMap, &decl, declList, codeList) && declarator(fInst, typeMap, decl, declList, codeList) &&
            (declaration_list(fInst, typeMap, declList, codeList) || 1) && compound_statement(fInst, typeMap, declList, codeList);
  if (!res) {
    fseek(fInst->fptr, fpos, SEEK_SET);
    return 0;
  } else {
    return 1;
  }
}

int declaration_list(FileInst* fInst, Map *typeMap, List *declList, List *codeList) {
  long int fpos = ftell(fInst->fptr);
  int res = declaration(fInst, typeMap, declList, codeList) && (declaration_list(fInst, typeMap, declList, codeList) || 1);
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