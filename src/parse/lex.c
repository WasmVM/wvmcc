#include "node.h"

static Node* stringLiteral(FileInst* fileInst, char thisChar) {
  if (thisChar == '\"' || thisChar == 'L' || thisChar == 'u' ||
      thisChar == 'U') {
    char* str = calloc(4096, sizeof(char));
    int strIndex = 0;
    int unitSize = 1;
    while (thisChar == '\"' || thisChar == 'L' || thisChar == 'u' ||
           thisChar == 'U') {
			// Decide unit size
			char specifier = '\0';
      switch (thisChar) {
        case 'L':
          unitSize = (unitSize < sizeof(wchar_t)) ? sizeof(wchar_t) : unitSize;
					specifier = 'L';
					thisChar = nextc(fileInst, NULL);
          break;
        case 'U':
          unitSize = (unitSize < 4) ? 4 : unitSize;
					specifier = 'U';
					thisChar = nextc(fileInst, NULL);
          break;
        case 'u':
          if ((thisChar = nextc(fileInst, NULL)) == '8') {
            unitSize = (unitSize < 4) ? 4 : unitSize;
						specifier = '8';
						thisChar = nextc(fileInst, NULL);
          } else {
            unitSize = (unitSize < 2) ? 2 : unitSize;
						specifier = 'u';
          }
          break;
        default:
          break;
      }
			// Ungetc if isn't string
			if(thisChar != '\"'){
				ungetc(thisChar, fileInst->fptr);
				if(specifier == '8'){
					ungetc('8', fileInst->fptr);
					specifier = 'u';
				}
				if(specifier != '\0'){
					ungetc(specifier, fileInst->fptr);
				}
        free(str);
        return NULL;
			}
			// Fill string
			thisChar = nextc(fileInst, NULL);
      while (strIndex < 4096 && thisChar != EOF &&
             thisChar != '\"' && thisChar != '\n') {
				int realChar = 0;
				// Escape sequence
        if (thisChar == '\\') {
					thisChar = nextc(fileInst, NULL);
					// Octal
          if (isdigit(thisChar) && thisChar != '8' && thisChar != '9') {
            for (int i = 0; thisChar != EOF && isdigit(thisChar) && thisChar != '8' &&
                   thisChar != '9' && (i < unitSize * 3); ++i) {
              realChar = realChar * 8 + (thisChar - '0');
							thisChar = nextc(fileInst, NULL);
            }
					// Hexadecimal & Universal
					} else if (thisChar == 'x' || thisChar == 'u' || thisChar == 'U') {
						int charSize = unitSize;
						switch(thisChar){
							case 'u':
								charSize = 2;
							break;
							case 'U':
								charSize = 4;
							break;
							default:
							break;
						}
            for (int i = 0; (thisChar = nextc(fileInst, NULL)) != EOF &&
                   (isdigit(thisChar) || (thisChar >= 'a' && thisChar <= 'f') ||
                    (thisChar >= 'A' && thisChar <= 'F')) && (i < charSize * 2); ++i) {
              if (thisChar >= 'a' && thisChar <= 'f') {
                realChar = realChar * 16 + (thisChar - 'a' + 10);
              } else if (thisChar >= 'A' && thisChar <= 'F') {
                realChar = realChar * 16 + (thisChar - 'A' + 10);
              } else {
                realChar = realChar * 16 + (thisChar - '0');
              }
            }
					// Simple
          } else {
            switch (thisChar) {
              case '\'':
              case '\"':
              case '\?':
              case '\\':
                realChar = thisChar;
                break;
              case 'a':
                realChar = '\a';
                break;
              case 'b':
                realChar = '\b';
                break;
              case 'f':
                realChar = '\f';
                break;
              case 'n':
                realChar = '\n';
                break;
              case 'r':
                realChar = '\r';
                break;
              case 't':
                realChar = '\t';
                break;
              case 'v':
                realChar = '\v';
                break;
              default:
                fprintf(stderr, WVMCC_ERR_EXPECT_STRING_LITERAL, getShortName(fileInst),
												fileInst->curline);
								free(str);
								return NULL;
            }
						thisChar = nextc(fileInst, NULL);
          }
        } else {
          realChar = thisChar;
					thisChar = nextc(fileInst, NULL);
        }
				// Fill character according to unitSize
				realChar &= (unitSize >= sizeof(int)) ? -1 : (1 << (unitSize * 8)) - 1;
				memcpy(str + strIndex, (char *)&realChar, unitSize);
				strIndex += unitSize;
      }
      if (thisChar == '\n' || thisChar == EOF) {
        fprintf(stderr, WVMCC_ERR_EXPECT_STRING_LITERAL, getShortName(fileInst),
                fileInst->curline);
        free(str);
        return NULL;
      }
      // Trim trailng space
      while (isspace(thisChar = nextc(fileInst, NULL))){
			}
    }
    // Alloc node
    Node* newToken = malloc(sizeof(Node));
    newToken->type = Tok_String;
    newToken->unitSize = unitSize;
		newToken->byteLen = strIndex;
    newToken->data.str = realloc(str, strIndex);
    return newToken;
  }
  return NULL;
}

static Node* integerConstant(FileInst* fileInst, char thisChar) {
	if(thisChar == '0'){
		if()
	}else if(isdigit(thisChar)){

	}
	return NULL;
}

Node* getToken(FileInst* fileInst) {
  char thisChar = nextc(fileInst, NULL);
  // Trim leading space
  while (isspace(thisChar)) {
    thisChar = nextc(fileInst, NULL);
  }
  Node* ret = NULL;
  // String literal
  if ((ret = stringLiteral(fileInst, thisChar)) != NULL) {
    return ret;
	// Integer constant
	} else if ((ret = integerConstant(fileInst, thisChar)) != NULL) {
    return ret;
  }
  // Unknown
  fprintf(stderr, WVMCC_ERR_UNKNOWN_TOKEN, getShortName(fileInst),
          fileInst->curline);
  return NULL;
}