#ifndef AST_H
#define AST_H

#include "common/io.h"

typedef enum ast_node_type {
	AST_BLOCK, AST_CALL
} ast_node_type_t;

typedef struct ast_node {
	ast_node_type_t type;
	string_t content;
	union {
		struct pair {
			struct ast_node *left;
			struct ast_node *right;
		};
		struct list {
			size_t child_count;
			size_t child_capacity;
			struct ast_node *children[];
		};
	};
} ast_node_t;

#endif // AST_H
