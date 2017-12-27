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
      if (thisChar != '\"') {
        ungetc(thisChar, fileInst->fptr);
        if (specifier == '8') {
          ungetc('8', fileInst->fptr);
          specifier = 'u';
        }
        if (specifier != '\0') {
          ungetc(specifier, fileInst->fptr);
        }
        free(str);
        return NULL;
      }
      // Fill string
      thisChar = nextc(fileInst, NULL);
      while (strIndex < 4096 && thisChar != EOF && thisChar != '\"' &&
             thisChar != '\n') {
        int realChar = 0;
        // Escape sequence
        if (thisChar == '\\') {
          thisChar = nextc(fileInst, NULL);
          // Octal
          if (isdigit(thisChar) && thisChar != '8' && thisChar != '9') {
            for (int i = 0;
                 thisChar != EOF && isdigit(thisChar) && thisChar != '8' &&
                 thisChar != '9' && (i < unitSize * 3);
                 ++i) {
              realChar = realChar * 8 + (thisChar - '0');
              thisChar = nextc(fileInst, NULL);
            }
            // Hexadecimal & Universal
          } else if (thisChar == 'x' || thisChar == 'u' || thisChar == 'U') {
            int charSize = unitSize;
            switch (thisChar) {
              case 'u':
                charSize = 2;
                break;
              case 'U':
                charSize = 4;
                break;
              default:
                break;
            }
            for (int i = 0;
                 (thisChar = nextc(fileInst, NULL)) != EOF &&
                 (isdigit(thisChar) || (thisChar >= 'a' && thisChar <= 'f') ||
                  (thisChar >= 'A' && thisChar <= 'F')) &&
                 (i < charSize * 2);
                 ++i) {
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
                fprintf(stderr, WVMCC_ERR_EXPECT_STRING_LITERAL,
                        getShortName(fileInst), fileInst->curline);
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
        memcpy(str + strIndex, (char*)&realChar, unitSize);
        strIndex += unitSize;
      }
      if (thisChar == '\n' || thisChar == EOF) {
        fprintf(stderr, WVMCC_ERR_EXPECT_STRING_LITERAL, getShortName(fileInst),
                fileInst->curline);
        free(str);
        return NULL;
      }
      // Trim trailng space
      while (isspace(thisChar = nextc(fileInst, NULL))) {
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
  // Get value
  unsigned long long int value = 0;
  if (thisChar == '0') {
    thisChar = nextc(fileInst, NULL);
    if (thisChar == 'x' || thisChar == 'X') {
      // hexadecimal
      while ((thisChar = nextc(fileInst, NULL)) != EOF &&
             (isdigit(thisChar) || (thisChar >= 'a' && thisChar <= 'f') ||
              (thisChar >= 'A' && thisChar <= 'F'))) {
        if (thisChar >= 'a' && thisChar <= 'f') {
          value = value * 16 + (thisChar - 'a' + 10);
        } else if (thisChar >= 'A' && thisChar <= 'F') {
          value = value * 16 + (thisChar - 'A' + 10);
        } else {
          value = value * 16 + (thisChar - '0');
        }
      }
    } else {
      // octal
      while (thisChar != EOF && isdigit(thisChar) && thisChar != '8' &&
             thisChar != '9') {
        value = value * 8 + (thisChar - '0');
        thisChar = nextc(fileInst, NULL);
      }
    }
  } else if (isdigit(thisChar)) {
    // decimal
    while (thisChar != EOF && isdigit(thisChar)) {
      value = value * 10 + (thisChar - '0');
      thisChar = nextc(fileInst, NULL);
    }
  } else {
    return NULL;
  }
	// Alloc node
	Node* newToken = malloc(sizeof(Node));
	newToken->type = Tok_Int;
	newToken->unitSize = sizeof(int);
	newToken->byteLen = sizeof(int);
	newToken->isSigned = 1;
	newToken->data.intVal = value;
	// Suffix
	if(thisChar == 'u' || thisChar == 'U'){
		newToken->isSigned = 0;
		thisChar = nextc(fileInst, NULL);
		if(thisChar == 'l' || thisChar == 'L'){
			thisChar = nextc(fileInst, NULL);
			if(thisChar == 'l' || thisChar == 'L'){
				// unsigned long long
				newToken->unitSize = sizeof(unsigned long long int);
				newToken->byteLen = sizeof(unsigned long long int);
			}else{
				// unsigned long
				ungetc(thisChar, fileInst->fptr);
				newToken->unitSize = sizeof(unsigned long int);
				newToken->byteLen = sizeof(unsigned long int);
			}
		}else{
			// unsigned
			ungetc(thisChar, fileInst->fptr);
		}
	}else if (thisChar == 'l' || thisChar == 'L'){
		thisChar = nextc(fileInst, NULL);
		if(thisChar == 'l' || thisChar == 'L'){
			// long long
			newToken->unitSize = sizeof(long long int);
			newToken->byteLen = sizeof(long long int);
			thisChar = nextc(fileInst, NULL);
		}else{
			// long
			newToken->unitSize = sizeof(long int);
			newToken->byteLen = sizeof(long int);
		}
		if(thisChar == 'u' || thisChar == 'U'){
			// unsigned
			newToken->isSigned = 0;
		}else{
			ungetc(thisChar, fileInst->fptr);
		}
	}
  return newToken;
}

static Node* floatingConstant(FileInst* fileInst, char thisChar) {
	// Get value
	double value = 0.0;
	int isHex = 0;
	if(thisChar == '0'){
		thisChar = nextc(fileInst, NULL);
		if(thisChar == 'x' || thisChar == 'X'){
			isHex = 1;
		}else{
			ungetc(thisChar, fileInst->fptr);
      thisChar = '0';
		}
	}
	if(isHex == 1){
		// hexadecimal
	}else{
		// decimal
		// integral part
		while (thisChar != EOF && isdigit(thisChar)) {
      value = value * 10 + (thisChar - '0');
      thisChar = nextc(fileInst, NULL);
    }
		// decimal point
		if(thisChar == '.'){
			// fraction part
      thisChar = nextc(fileInst, NULL);
      for (double i = 0.1; thisChar != EOF && isdigit(thisChar); i *= 0.1) {
        value += (thisChar - '0') * i;
        thisChar = nextc(fileInst, NULL);
      }
      // exponent part [optional]
      if(thisChar == 'e' || thisChar == 'E'){
        thisChar = nextc(fileInst, NULL);
        char sign = '+';
        
      }
		}else if(thisChar == 'e' || thisChar == 'E'){
      // exponent part [essential]
    }
	}
	return NULL;
}

Node* getToken(FileInst* fileInst) {
  char thisChar = nextc(fileInst, NULL);
  // Trim leading space
  while (isspace(thisChar)) {
    thisChar = nextc(fileInst, NULL);
  }
  long int fpos = ftell(fileInst->fptr);
	// String literal
  Node* ret = stringLiteral(fileInst, thisChar);
  // Floating constant
  if (ret == NULL) {
    fseek(fileInst->fptr, fpos, SEEK_SET);
		ret = floatingConstant(fileInst, thisChar);
  }
	// Integer constant
  if (ret == NULL) {
    fseek(fileInst->fptr, fpos, SEEK_SET);
		ret = integerConstant(fileInst, thisChar);
  }
  // End of file
	if(ret == NULL && thisChar == EOF){
		ret = malloc(sizeof(Node));
		ret->type = Tok_EOF;
	}
  return ret;
}