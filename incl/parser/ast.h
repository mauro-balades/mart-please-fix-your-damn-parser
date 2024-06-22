#ifndef AST_H
#define AST_H

#include "common/arena.h"
#include "common/io.h"

typedef enum {
	NODE_BLOCK,
	NODE_NUMBER
} NodeType;

typedef struct {
	NodeType type;
} Node;

typedef struct {
	Node 				 base;
	size_t 			 size;
	void *			 children;
} BlockNode;

typedef struct {
	Node 				 base;
	unsigned int value;
} NumberNode;

Node* ast_new_block(void* children, size_t size);
Node* ast_new_number(unsigned int value);

// Other functions
void ast_node_append(void** children, size_t* size, Node* node);

#endif // AST_H
