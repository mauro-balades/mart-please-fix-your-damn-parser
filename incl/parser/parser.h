#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "parser/ast.h"

typedef struct {
  char *input;
  size_t pos;
  Node *ast;
} Parser;

void parser_start(void);

#endif // PARSER_H