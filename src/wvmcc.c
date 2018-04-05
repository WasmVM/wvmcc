#include <errno.h>
#include <stdio.h>

#include "fileInst.h"
#include "parse/rules.h"

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
  List *declList = listNew();
  // Parse
  int err = startParse(fInst, declList);
  
  // Clean
  fclose(fout);
  if (err) {
    remove(argv[2]);
    return -1;
  }
  fileInstFree(&fInst);
  return 0;
}