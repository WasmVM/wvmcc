#include <errno.h>
#include <stdio.h>

int compile(FILE *fin, FILE *fout){

	return 0;
}

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
  FILE* fin = fopen(argv[1], "r");
  if (errno) {
    perror(argv[1]);
    return -2;
  }
  // Open output file
  FILE* fout = fopen(argv[2], "wb");
  if (errno) {
    perror(argv[2]);
    return -3;
  }
  // Start compiling
  int err = compile(fin, fout);
  // Close output
  fclose(fout);
  if (err) {
    remove(argv[2]);
    return -1;
  }
  return 0;
}