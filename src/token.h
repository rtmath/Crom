#ifndef TOKEN_H
#define TOKEN_H

#include "token_type.h"

typedef struct {
  TokenType type;
  const char *position_in_source;
  int length;

  // For helpful error messages
  const char *from_filename;
  int on_line;
  int line_x_offset;
} Token;

void InlinePrintToken(Token t);
void PrintToken(Token t);
void PrintTokenVerbose(Token t);

#endif
