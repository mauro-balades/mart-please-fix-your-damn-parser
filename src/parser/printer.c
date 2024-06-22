
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
  }
}
