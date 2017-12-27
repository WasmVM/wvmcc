#include <errno.h>
#include <stdio.h>

#include "fileInst.h"
#include "parse/node.h"

int main(int argc, char* argv[]) {
  // Check argc
  if (argc < 3) {
    if (argc < 2) {
      fprintf(stderr, "[CC] No input file.\n");
    } else {
      fprintf(stderr, "[CC] No output file.\n");
    }
    return -1;
  }
  // Open input file
  FileInst *fInst = fileInstNew(argv[1]);
  // Open output file
  FILE* fout = fopen(argv[2], "wb");
  if (errno) {
    perror(argv[2]);
    return -3;
  }
  // Start compiling
  int err = 0;
  Node *token = NULL;
  while((token = getToken(fInst)) != NULL){
    switch(token->type){
      case Tok_String: 
        printf("<String>");
        for(int i = 0; i < token->byteLen; ++i){
          printf(" %02x", (char)token->data.str[i]);
        }
        printf("\n");
      break;
      case Tok_Int: 
        printf("<Integer> %llu\n", token->data.intVal);
      break;
      default: 
      break;
    }
  }
  // Clean
  fclose(fout);
  if (err) {
    remove(argv[2]);
    return -1;
  }
  fileInstFree(&fInst);
  return 0;
}