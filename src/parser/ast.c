
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


Node* ast_new_binary_op(Node* left, Node* right, token_type_t op) {
  BinaryOpNode* binary_op = malloc(sizeof(BinaryOpNode));
  binary_op->base.type = NODE_BINARY_OP;
  binary_op->left = left;
  binary_op->right = right;
  binary_op->op = op;
  return (Node*)binary_op;
}

void ast_node_append(void** children, size_t* size, Node* node) {
  *children = realloc(*children, (*size + 1) * sizeof(Node*));
  ((Node**)*children)[*size] = node;
  (*size)++;
}

void ast_node_erase(void** children, size_t* size, size_t index) {
  for (size_t i = index; i < *size - 1; i++) {
    ((Node**)*children)[i] = ((Node**)*children)[i + 1];
  }
  *children = realloc(*children, (*size - 1) * sizeof(Node*));
  (*size)--;
}
