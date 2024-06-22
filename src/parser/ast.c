
#include "parser/ast.h"

#include <stdlib.h>

Node* ast_new_block(void* children, size_t size) {
  BlockNode* block = malloc(sizeof(BlockNode));
  block->base.type = NODE_BLOCK;
  block->size = size;
  block->children = children;
  return (Node*)block;
}

Node* ast_new_number(unsigned int value) {
  NumberNode* number = malloc(sizeof(NumberNode));
  number->base.type = NODE_NUMBER;
  number->value = value;
  return (Node*)number;
}

void ast_node_append(void** children, size_t* size, Node* node) {
  *children = realloc(*children, (*size + 1) * sizeof(Node*));
  ((Node**)*children)[*size] = node;
  (*size)++;
}
