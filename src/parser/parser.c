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

static Node* parse_block(void);
static Node* parse_statement(void);
static Node* parse_expression(void);
static Node* parse_number(void);
bool				 parse_operator(void** children, size_t* size);

static void error(const char *fmt, ...);

static void error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

bool parse_operator(void** children, size_t* size) {
	bool valid = true;
	// TODO: Consume operator node here, having left and right to NULL!
	return valid;
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
	// TODO: Fix operator precedence here!
	return ((Node**)children)[0];
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

