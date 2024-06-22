
#include "parser/printer.h"
#include "parser/ast.h"

void ast_print(Node* node) {
  switch (node->type) {
    case NODE_BLOCK: {
      BlockNode* block = (BlockNode*)node;
      printf("BLOCK {\n");
      for (size_t i = 0; i < block->size; i++) {
        printf("  ");
        ast_print(((Node**)block->children)[i]);
        if (i < block->size - 1) {
          printf(",\n");
        }
      }
      printf("\n}\n");
      break;
    }
    case NODE_NUMBER: {
      NumberNode* number = (NumberNode*)node;
      printf("%u", number->value);
      break;
    }
    case NODE_BINARY_OP: {
      BinaryOpNode* binary_op = (BinaryOpNode*)node;
      printf("(");
      ast_print(binary_op->left);
      printf(" ");
      switch (binary_op->op) {
        case TOK_OP_PLUS:
          printf("+");
          break;
        case TOK_OP_MINUS:
          printf("-");
          break;
        case TOK_OP_MULT:
          printf("*");
          break;
        case TOK_OP_DIV:
          printf("/");
          break;
      }
      printf(" ");
      ast_print(binary_op->right);
      printf(")");
      break;
    }
  }
}
