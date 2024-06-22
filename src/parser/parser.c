#include "ast.h"
#include "parser.h"
#include "printer.h"

#include "common/io.h"
#include "lexer/lexer.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NEXT() (lexer_next())
#define CURRENT() (lexer_peek())

#define IS(t) (CURRENT()->type == t)

#define KILL_ME() assert(false);

static Node* 		parse_block(void);
static Node* 		parse_statement(void);
static Node* 		parse_expression(void);
static Node* 		parse_number(void);
static unsigned get_precedence(token_type_t type);
static Node* 		fix_expression(void** children, size_t* size);
bool				 		parse_operator(void** children, size_t* size);

static void error(const char *fmt, ...);

static void error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

static unsigned int get_precedence(token_type_t type) {
	switch (type) {
		case TOK_OP_MULT:
		case TOK_OP_DIV:
			return 1;
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
			return 2;
		default:
			error("Invalid operator %s\n", token_type_strs[type]);
	}
}

static Node* fix_expression(void** children, size_t* size) {
	if (*size == 1) {
		return ((Node**)*children)[0];
	}
	while (*size > 1) {
		int next = -1;
		int min_precedence = 0xFFFFF;
		bool unary = false;
		for (size_t i = 0; i < *size; i++) {
			Node* node = ((Node**)*children)[i];
			if (node->type == NODE_BINARY_OP) {
				BinaryOpNode* binary_op = (BinaryOpNode*)node;
				if (!binary_op->is_op) continue;
				unsigned int precedence = get_precedence(binary_op->op);
				if (precedence < min_precedence) {
					min_precedence = precedence;
					next = i;
					unary = false; // TODO: unary operators
				}
			}
		}
		assert(next != -1);
		assert(!unary && "Unary operators not supported yet");
		assert(next >= 1 && next < *size - 1);
		BinaryOpNode* binary_op = (BinaryOpNode*)((Node**)*children)[next];
		Node* left = ((Node**)*children)[next - 1];
		Node* right = ((Node**)*children)[next + 1];
		binary_op->left = left;
		binary_op->right = right;
		binary_op->is_op = false;
		// Remove elements at next - 1 and next + 1
		ast_node_erase(children, size, next + 1);
		ast_node_erase(children, size, next - 1);
	}
	return ((Node**)*children)[0];
}

bool parse_operator(void** children, size_t* size) {
	bool valid = true;
	switch (CURRENT()->type) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TOK_OP_MULT:
		case TOK_OP_DIV:
			break;
		default:
			valid = false;
	}
	if (valid) {
		Node* op = ast_new_binary_op(NULL, NULL, CURRENT()->type);
		((BinaryOpNode*)op)->is_op = true;
		ast_node_append(children, size, op);
		NEXT();
	}
	return !valid;
}

static Node* parse_number(void) {
	assert(IS(TOK_LIT_NUM));
	unsigned int value = atoi(CURRENT()->content.string);
	return ast_new_number(value);
}

static Node* parse_expression(void) {
	void* children = NULL;
	size_t size = 0;
	while(true) {
		Node* expr = NULL;
		switch (CURRENT()->type) {
			case TOK_LIT_NUM:
				expr = parse_number();
				break;
			default:
				error("Unexpected token %s\n", token_type_strs[CURRENT()->type]);
		}
		NEXT();
		while (true) {
			/*if (IS(TOK_OPEN_ROUND)) {
// parse calls and shit here
			}	else*/ {
				break;
			}		
		}
		ast_node_append(&children, &size, expr);
		if (parse_operator(&children, &size)) {
			break;
		}
	}
	return fix_expression(&children, &size);
}

static Node* parse_statement(void) {
	switch (CURRENT()->type) {
		default:
			return parse_expression();
	}
}

static Node* parse_block(void) {
	size_t size = 0;
	void *children = NULL;
	while(true) {
		switch (CURRENT()->type) {
			case TOK_KW_END:
			case TOK_EOF:
				return ast_new_block(children, size);
			default:
				ast_node_append(&children, &size, parse_statement());
		}
	}
	KILL_ME();
}

void parser_start(void) {
	Parser *parser = malloc(sizeof(Parser));
	parser->ast = parse_block();
	ast_print(parser->ast);
}

