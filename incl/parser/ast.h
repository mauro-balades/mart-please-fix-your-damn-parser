#ifndef AST_H
#define AST_H

#include "common/arena.h"
#include "common/io.h"

#include "lexer/lexer.h"	// token_t

typedef enum {
	NODE_BLOCK,
	NODE_NUMBER,
	NODE_BINARY_OP
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

typedef struct {
	Node 				 base;
	Node *			 left;
	Node *			 right;
	token_type_t op;
	bool				 is_op;
} BinaryOpNode;

Node* ast_new_block(void* children, size_t size);
Node* ast_new_number(unsigned int value);
Node* ast_new_binary_op(Node* left, Node* right, token_type_t op);

// Other functions
void ast_node_append(void** children, size_t* size, Node* node);
void ast_node_erase(void** children, size_t* size, size_t index);

#endif // AST_H
