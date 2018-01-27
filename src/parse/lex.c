#include "token.h"

static Token* stringLiteral(FileInst* fileInst, char thisChar) {
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
        int chrSize = (unitSize >= sizeof(int)) ? sizeof(int) : unitSize;
        realChar &= (unitSize >= sizeof(int)) ? -1 : (1 << (unitSize * 8)) - 1;
        memcpy(str + strIndex, (char*)&realChar, chrSize);
        strIndex += chrSize;
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
    Token* newToken = malloc(sizeof(Token));
    newToken->type = Tok_String;
    newToken->unitSize = unitSize;
    newToken->byteLen = strIndex;
    newToken->data.str = realloc(str, strIndex);
    return newToken;
  }
  return NULL;
}

static Token* punctuator(FileInst* fileInst, char thisChar) {
  Punct newPunct;
  char nextChar;
  switch (thisChar) {
    // single state
    case '[':
      newPunct = Punct_brackL;
      break;
    case ']':
      newPunct = Punct_brackR;
      break;
    case '(':
      newPunct = Punct_paranL;
      break;
    case ')':
      newPunct = Punct_paranR;
      break;
    case '{':
      newPunct = Punct_braceL;
      break;
    case '}':
      newPunct = Punct_braceR;
      break;
    case '?':
      newPunct = Punct_ques;
      break;
    case ';':
      newPunct = Punct_semi;
      break;
    case '~':
      newPunct = Punct_tilde;
      break;
    case ',':
      newPunct = Punct_comma;
      break;
    // 2 states
    case '*':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '=') {
        newPunct = Punct_ass_mult;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_aster;
      }
      break;
    case '!':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '=') {
        newPunct = Punct_neq;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_exclm;
      }
      break;
    case '/':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '=') {
        newPunct = Punct_ass_div;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_slash;
      }
      break;
    case '=':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '=') {
        newPunct = Punct_eq;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_assign;
      }
      break;
    case '^':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '=') {
        newPunct = Punct_ass_xor;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_caret;
      }
      break;
    case ':':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '>') {
        newPunct = Punct_brackR;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_colon;
      }
      break;
    case '.':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '.') {
        nextChar = nextc(fileInst, NULL);
        if (nextChar == '.') {
          newPunct = Punct_ellips;
        } else {
          ungetc('.', fileInst->fptr);
          ungetc(nextChar, fileInst->fptr);
          newPunct = Punct_dot;
        }
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_dot;
      }
      break;
    // 3 states
    case '&':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '&') {
        newPunct = Punct_and;
      } else if (nextChar == '=') {
        newPunct = Punct_ass_and;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_amp;
      }
      break;
    case '|':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '|') {
        newPunct = Punct_or;
      } else if (nextChar == '=') {
        newPunct = Punct_ass_or;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_vbar;
      }
      break;
    case '+':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '+') {
        newPunct = Punct_inc;
      } else if (nextChar == '=') {
        newPunct = Punct_ass_plus;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_plus;
      }
      break;
    case '%':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '>') {
        newPunct = Punct_braceR;
      } else if (nextChar == '=') {
        newPunct = Punct_ass_mod;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_percent;
      }
      break;
    // 4 states
    case '-':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '-') {
        newPunct = Punct_dec;
      } else if (nextChar == '>') {
        newPunct = Punct_arrow;
      } else if (nextChar == '=') {
        newPunct = Punct_ass_minus;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_minus;
      }
      break;
    case '>':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '>') {
        nextChar = nextc(fileInst, NULL);
        if (nextChar == '=') {
          newPunct = Punct_ass_shr;
        } else {
          ungetc(nextChar, fileInst->fptr);
          newPunct = Punct_shiftR;
        }
      } else if (nextChar == '=') {
        newPunct = Punct_ge;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_gt;
      }
      break;
    // 6 states
    case '<':
      nextChar = nextc(fileInst, NULL);
      if (nextChar == '<') {
        nextChar = nextc(fileInst, NULL);
        if (nextChar == '=') {
          newPunct = Punct_ass_shl;
        } else {
          ungetc(nextChar, fileInst->fptr);
          newPunct = Punct_shiftL;
        }
      } else if (nextChar == ':') {
        newPunct = Punct_brackL;
      } else if (nextChar == '%') {
        newPunct = Punct_braceL;
      } else if (nextChar == '=') {
        newPunct = Punct_le;
      } else {
        ungetc(nextChar, fileInst->fptr);
        newPunct = Punct_lt;
      }
      break;
    default:
      return NULL;
  }
  // Alloc node
  Token* newToken = malloc(sizeof(Token));
  newToken->type = Tok_Punct;
  newToken->data.punct = newPunct;
  return newToken;
}

static Token* characterConstant(FileInst* fileInst, char thisChar) {
  if (thisChar == '\'' || thisChar == 'L' || thisChar == 'u' ||
      thisChar == 'U') {
    // Decide unit size
    int unitSize = 1;
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
        unitSize = (unitSize < 2) ? 2 : unitSize;
        specifier = 'u';
        thisChar = nextc(fileInst, NULL);
        break;
      default:
        break;
    }
    // Ungetc if isn't string
    if (thisChar != '\'') {
      ungetc(thisChar, fileInst->fptr);
      if (specifier != '\0') {
        ungetc(specifier, fileInst->fptr);
      }
      return NULL;
    }
    // Fill string
    thisChar = nextc(fileInst, NULL);
    char* chr = calloc(unitSize, 1);
    for (int byteIndex = 0,
             chrSize = (unitSize >= sizeof(int)) ? sizeof(int) : unitSize;
         byteIndex < unitSize && thisChar != EOF && thisChar != '\'' &&
         thisChar != '\n';
         byteIndex += chrSize) {
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
              free(chr);
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
      memcpy(chr + byteIndex, (char*)&realChar, chrSize);
    }
    // Ignore exceed value
    while (thisChar != '\'') {
      // Error if not close
      if (thisChar == '\n' || thisChar == EOF) {
        fprintf(stderr, WVMCC_ERR_EXPECT_CHARACTER_CONSTANT,
                getShortName(fileInst), fileInst->curline);
        free(chr);
        return NULL;
      }
      thisChar = nextc(fileInst, NULL);
    }
    // Trim trailng space
    while (isspace(thisChar = nextc(fileInst, NULL))) {
    }
    // Alloc node
    Token* newToken = malloc(sizeof(Token));
    newToken->type = Tok_Char;
    newToken->unitSize = unitSize;
    newToken->byteLen = unitSize;
    newToken->data.str = chr;
    return newToken;
  }
  return NULL;
}

static Token* integerConstant(FileInst* fileInst, char thisChar) {
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
  Token* newToken = malloc(sizeof(Token));
  newToken->type = Tok_Int;
  newToken->unitSize = sizeof(int);
  newToken->byteLen = sizeof(int);
  newToken->isSigned = 1;
  newToken->data.intVal = value;
  // Suffix
  if (thisChar == 'u' || thisChar == 'U') {
    newToken->isSigned = 0;
    thisChar = nextc(fileInst, NULL);
    if (thisChar == 'l' || thisChar == 'L') {
      thisChar = nextc(fileInst, NULL);
      if (thisChar == 'l' || thisChar == 'L') {
        // unsigned long long
        newToken->unitSize = sizeof(unsigned long long int);
        newToken->byteLen = sizeof(unsigned long long int);
      } else {
        // unsigned long
        ungetc(thisChar, fileInst->fptr);
        newToken->unitSize = sizeof(unsigned long int);
        newToken->byteLen = sizeof(unsigned long int);
      }
    } else {
      // unsigned
      ungetc(thisChar, fileInst->fptr);
    }
  } else if (thisChar == 'l' || thisChar == 'L') {
    thisChar = nextc(fileInst, NULL);
    if (thisChar == 'l' || thisChar == 'L') {
      // long long
      newToken->unitSize = sizeof(long long int);
      newToken->byteLen = sizeof(long long int);
      thisChar = nextc(fileInst, NULL);
    } else {
      // long
      newToken->unitSize = sizeof(long int);
      newToken->byteLen = sizeof(long int);
    }
    if (thisChar == 'u' || thisChar == 'U') {
      // unsigned
      newToken->isSigned = 0;
    } else {
      ungetc(thisChar, fileInst->fptr);
    }
  } else {
    ungetc(thisChar, fileInst->fptr);
  }
  return newToken;
}

static Token* floatingConstant(FileInst* fileInst, char thisChar) {
  // Get value
  double value = 0.0;
  int isHex = 0;
  if (thisChar == '0') {
    thisChar = nextc(fileInst, NULL);
    if (thisChar == 'x' || thisChar == 'X') {
      isHex = 1;
      thisChar = nextc(fileInst, NULL);
    } else {
      ungetc(thisChar, fileInst->fptr);
      thisChar = '0';
    }
  }
  if (isHex == 1) {
    // hexadecimal
    // integral part
    while (thisChar != EOF &&
           (isdigit(thisChar) || (thisChar >= 'a' && thisChar <= 'f') ||
            (thisChar >= 'A' && thisChar <= 'F'))) {
      if (thisChar >= 'a' && thisChar <= 'f') {
        value = value * 16 + (thisChar - 'a' + 10);
      } else if (thisChar >= 'A' && thisChar <= 'F') {
        value = value * 16 + (thisChar - 'A' + 10);
      } else {
        value = value * 16 + (thisChar - '0');
      }
      thisChar = nextc(fileInst, NULL);
    }
    // decimal point
    if (thisChar == '.') {
      // fraction part
      thisChar = nextc(fileInst, NULL);
      for (double i = 0.0625;
           thisChar != EOF &&
           (isdigit(thisChar) || (thisChar >= 'a' && thisChar <= 'f') ||
            (thisChar >= 'A' && thisChar <= 'F'));
           i *= 0.0625) {
        if (thisChar >= 'a' && thisChar <= 'f') {
          value += (thisChar - 'a' + 10) * i;
        } else if (thisChar >= 'A' && thisChar <= 'F') {
          value += (thisChar - 'A' + 10) * i;
        } else {
          value += (thisChar - '0') * i;
        }
        thisChar = nextc(fileInst, NULL);
      }
      // exponent part [optional]
      if (thisChar == 'p' || thisChar == 'P') {
        thisChar = nextc(fileInst, NULL);
        char sign = '+';
        if (thisChar == '+' || thisChar == '-') {
          sign = thisChar;
          thisChar = nextc(fileInst, NULL);
        }
        int expo = 0;
        while (thisChar != EOF && isdigit(thisChar)) {
          expo = expo * 10 + (thisChar - '0');
          thisChar = nextc(fileInst, NULL);
        }
        ungetc(thisChar, fileInst->fptr);
        if (sign == '-') {
          expo *= -1.0;
        }
        value *= pow(2, expo);
      }
    } else if (thisChar == 'p' || thisChar == 'P') {
      // exponent part [essential]
      thisChar = nextc(fileInst, NULL);
      char sign = '+';
      if (thisChar == '+' || thisChar == '-') {
        sign = thisChar;
        thisChar = nextc(fileInst, NULL);
      }
      int expo = 0;
      while (thisChar != EOF && isdigit(thisChar)) {
        expo = expo * 10 + (thisChar - '0');
        thisChar = nextc(fileInst, NULL);
      }
      ungetc(thisChar, fileInst->fptr);
      if (sign == '-') {
        expo *= -1.0;
      }
      value *= pow(2, expo);
    } else {
      return NULL;
    }
  } else {
    // decimal
    // integral part
    while (thisChar != EOF && isdigit(thisChar)) {
      value = value * 10 + (thisChar - '0');
      thisChar = nextc(fileInst, NULL);
    }
    // decimal point
    if (thisChar == '.') {
      // fraction part
      thisChar = nextc(fileInst, NULL);
      for (double i = 0.1; thisChar != EOF && isdigit(thisChar); i *= 0.1) {
        value += (thisChar - '0') * i;
        thisChar = nextc(fileInst, NULL);
      }
      // exponent part [optional]
      if (thisChar == 'e' || thisChar == 'E') {
        thisChar = nextc(fileInst, NULL);
        char sign = '+';
        if (thisChar == '+' || thisChar == '-') {
          sign = thisChar;
          thisChar = nextc(fileInst, NULL);
        }
        int expo = 0;
        while (thisChar != EOF && isdigit(thisChar)) {
          expo = expo * 10 + (thisChar - '0');
          thisChar = nextc(fileInst, NULL);
        }
        ungetc(thisChar, fileInst->fptr);
        if (sign == '-') {
          expo *= -1.0;
        }
        value *= pow(10, expo);
      }
    } else if (thisChar == 'e' || thisChar == 'E') {
      // exponent part [essential]
      thisChar = nextc(fileInst, NULL);
      char sign = '+';
      if (thisChar == '+' || thisChar == '-') {
        sign = thisChar;
        thisChar = nextc(fileInst, NULL);
      }
      int expo = 0;
      while (thisChar != EOF && isdigit(thisChar)) {
        expo = expo * 10 + (thisChar - '0');
        thisChar = nextc(fileInst, NULL);
      }
      ungetc(thisChar, fileInst->fptr);
      if (sign == '-') {
        expo *= -1.0;
      }
      value *= pow(10, expo);
    } else {
      return NULL;
    }
  }
  // Alloc node
  Token* newToken = malloc(sizeof(Token));
  newToken->type = Tok_Float;
  newToken->data.floatVal = value;
  newToken->unitSize = sizeof(double);
  newToken->byteLen = sizeof(double);
  // Suffix
  if (thisChar == 'f' || thisChar == 'F') {
    newToken->unitSize = sizeof(float);
    newToken->byteLen = sizeof(float);
  } else if (thisChar == 'l' || thisChar == 'L') {
    // WASM only have f64, long double is ignored
    thisChar = nextc(fileInst, NULL);
  }
  return newToken;
}

static void keyword(Token *node, char *identifier) {
  if(!strcmp(identifier, "auto")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_auto;
  }else if(!strcmp(identifier, "break")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_break;
  }else if(!strcmp(identifier, "case")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_case;
  }else if(!strcmp(identifier, "char")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_char;
  }else if(!strcmp(identifier, "const")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_const;
  }else if(!strcmp(identifier, "continue")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_continue;
  }else if(!strcmp(identifier, "default")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_default;
  }else if(!strcmp(identifier, "do")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_do;
  }else if(!strcmp(identifier, "double")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_double;
  }else if(!strcmp(identifier, "else")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_else;
  }else if(!strcmp(identifier, "enum")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_enum;
  }else if(!strcmp(identifier, "extern")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_extern;
  }else if(!strcmp(identifier, "float")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_float;
  }else if(!strcmp(identifier, "for")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_for;
  }else if(!strcmp(identifier, "goto")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_goto;
  }else if(!strcmp(identifier, "if")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_if;
  }else if(!strcmp(identifier, "inline")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_inline;
  }else if(!strcmp(identifier, "int")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_int;
  }else if(!strcmp(identifier, "long")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_long;
  }else if(!strcmp(identifier, "register")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_register;
  }else if(!strcmp(identifier, "restrict")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_restrict;
  }else if(!strcmp(identifier, "return")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_return;
  }else if(!strcmp(identifier, "short")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_short;
  }else if(!strcmp(identifier, "signed")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_signed;
  }else if(!strcmp(identifier, "sizeof")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_sizeof;
  }else if(!strcmp(identifier, "static")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_static;
  }else if(!strcmp(identifier, "struct")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_struct;
  }else if(!strcmp(identifier, "switch")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_switch;
  }else if(!strcmp(identifier, "typedef")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_typedef;
  }else if(!strcmp(identifier, "union")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_union;
  }else if(!strcmp(identifier, "unsigned")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_unsigned;
  }else if(!strcmp(identifier, "void")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_void;
  }else if(!strcmp(identifier, "volatile")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_volatile;
  }else if(!strcmp(identifier, "while")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_while;
  }else if(!strcmp(identifier, "_Alignas")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Alignas;
  }else if(!strcmp(identifier, "_Alignof")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Alignof;
  }else if(!strcmp(identifier, "_Atomic")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Atomic;
  }else if(!strcmp(identifier, "_Bool")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Bool;
  }else if(!strcmp(identifier, "_Complex")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Complex;
  }else if(!strcmp(identifier, "_Generic")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Generic;
  }else if(!strcmp(identifier, "_Imaginary")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Imaginary;
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Noreturn;
  }else if(!strcmp(identifier, "_Static_assert")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Static_assert;
  }else if(!strcmp(identifier, "_Thread_local")){
    node->type = Tok_Keyword;
    node->data.keyword = Keyw_Thread_local;
  }
}

static Token* identifier(FileInst* fileInst, char thisChar) {
  char* ident = (char*)calloc(65, sizeof(char));
  if (isalpha(thisChar) || thisChar == '_') {
    ident[0] = thisChar;
    for (int i = 1; i <= 64; ++i) {
      if (i == 64) {
        return NULL;
      }
      thisChar = nextc(fileInst, NULL);
      if (!isalnum(thisChar) && thisChar != '_') {
        ungetc(thisChar, fileInst->fptr);
        break;
      }
      ident[i] = thisChar;
    }
  } else {
    free(ident);
    return NULL;
  }
  ident = (char*)realloc(ident, strlen(ident) + 1);
  // Alloc node
  Token* newToken = malloc(sizeof(Token));
  newToken->type = Tok_Ident;
  keyword(newToken, ident);
  if(newToken->type != Tok_Keyword){
    newToken->data.str = ident;
    newToken->unitSize = sizeof(char);
    newToken->byteLen = sizeof(char) * strlen(ident);
  }
  return newToken;
}

Token* getToken(FileInst* fileInst) {
  char thisChar = nextc(fileInst, NULL);
  // Trim leading space
  while (isspace(thisChar)) {
    thisChar = nextc(fileInst, NULL);
  }
  long int fpos = ftell(fileInst->fptr);
  // String literal
  Token* ret = stringLiteral(fileInst, thisChar);
  // Character constant
  if (ret == NULL) {
    fseek(fileInst->fptr, fpos, SEEK_SET);
    ret = characterConstant(fileInst, thisChar);
  }
  // Identifier
  if (ret == NULL) {
    fseek(fileInst->fptr, fpos, SEEK_SET);
    ret = identifier(fileInst, thisChar);
  }
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
  // Punctuator
  if (ret == NULL) {
    fseek(fileInst->fptr, fpos, SEEK_SET);
    ret = punctuator(fileInst, thisChar);
  }
  // End of file
  if (ret == NULL && thisChar == EOF) {
    ret = malloc(sizeof(Token));
    ret->type = Tok_EOF;
  }
  return ret;
}