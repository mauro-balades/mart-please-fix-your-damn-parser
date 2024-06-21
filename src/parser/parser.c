#include "ast.h"
#include "lookaheads.h"
#include "parser.h"

#include "common/io.h"
#include "lexer/lexer.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum se_variant {
	OUTER_STMT_EXPR,
	DELIM_STMT_EXPR,
	INNER_STMT_EXPR,
} se_variant_t;

static struct parser_state {
	ast_t ast;
} ps;

// Internal Functions (Helpers) //

static void _panic_common(token_t *problem) {
	size_t line = 1;
	char *file_base = lexer_get_src().string;
	char *problem_base = problem->content.string;
	char *last_nl = file_base - 1;

	for(char *c = file_base; c < problem_base; c++)
		if(*c == '\n') line++, last_nl = c;

	printf("[%lu:%lu] Errant token encountered: ", line, (uintptr_t) (problem_base - last_nl));
	if(problem->type == TOK_EOF) printf("EOF");
	else printf("\"%.*s\"", (int) problem->content.size, problem_base);
}

static void _panic(token_t *problem) {
	_panic_common(problem);
	putchar('\n');
	exit(EXIT_FAILURE);
}

static void _panic_expect(token_t *problem, const char *message) {
	_panic_common(problem);
	printf(", expected: %s\n", message);
	exit(EXIT_FAILURE);
}

static token_t *_expect(token_type_t type) {
	token_t *next = CONSUME;
	if(next->type != type) {
		_panic_common(next);
		printf(", expected: %s\n", token_type_strs[type]);
		exit(EXIT_FAILURE);
	}
	return next;
}

// Internal Function Decls (Non-Terminals) //

static ast_node_t *_nt_block(void);
static ast_node_t *_nt_var_init(ast_node_t **parent);
static void _nt_var_expr_next(ast_node_t **parent);
static void _nt_var_stmt_next(ast_node_t **parent);
static ast_node_t *_nt_type(void);
static ast_node_t *_nt_stmt_common(void);
static void _nt_else(ast_node_t **parent);
static ast_node_t *_nt_outer_stmt(void);
static ast_node_t *_nt_inner_stmt(void);
static ast_node_t *_nt_outer_stmt_expr(void);
static ast_node_t *_nt_delim_stmt_expr(void);
static ast_node_t *_nt_inner_stmt_expr(void);

static ast_node_t *_nt_prec_0(void);
static ast_node_t *_nt_prec_0_(void);
static ast_node_t *_nt_prec_1(void);
static ast_node_t *_nt_prec_1_(void);
static ast_node_t *_nt_prec_2(void);
static ast_node_t *_nt_prec_2_(void);
static ast_node_t *_nt_unaries_2(void);
static ast_node_t *_nt_prec_3(void);
static ast_node_t *_nt_prec_3_(void);
static ast_node_t *_nt_prec_4(void);
static ast_node_t *_nt_prec_4_(void);
static ast_node_t *_nt_prec_5(void);
static ast_node_t *_nt_unaries_5(void);
static ast_node_t *_nt_term(void);
static ast_node_t *_nt_term_(ast_node_t *child);
static ast_node_t *_nt_func(ast_node_t *child);
static void _nt_func_(ast_node_t **parent);

// Internal Function Defs (Non-Terminal Helpers) //

static void _nth_vardecl(ast_node_t **parent) {
	string_t ident_str = _expect(TOK_IDENT)->content;
	string_t assign_str = _expect(TOK_OP_ASSIGN)->content;
	ast_node_t *assign = ast_pnode_new(&ps.ast, AST_OP_BINARY, assign_str);
	*parent = ast_lnode_add(&ps.ast, *parent, assign);
	ast_pnode_left(assign, ast_pnode_new(&ps.ast, AST_IDENT, ident_str));
	ast_pnode_right(assign, _nt_var_init(parent));
}

// Internal Functions Defs (Non-Terminals) //

static ast_node_t *_nt_block(void) {
	ast_node_t *node = ast_lnode_new(&ps.ast, 4, AST_BLOCK, EMPTY_STRING);
loop:
	switch(PEEK) {
		case TOK_KW_VAR: ;
			ast_node_t *vardecl = ast_lnode_new(&ps.ast, 4, AST_VAR, CONSUME->content);
			vardecl = ast_lnode_add(&ps.ast, vardecl, _nt_type());
			_nth_vardecl(&vardecl);
			node = ast_lnode_add(&ps.ast, node, vardecl);
			goto loop;
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			node = ast_lnode_add(&ps.ast, node, _nt_outer_stmt_expr());
			goto loop;
		case TOK_KW_END:
		case TOK_KW_ELIF:
		case TOK_KW_ELSE:
		case TOK_EOF:
			break;
		default: _panic_expect(CONSUME, "\"var\", statement or expression");
	}
	return node;
}

static ast_node_t *_nt_var_init(ast_node_t **parent) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			node = _nt_outer_stmt();
			_nt_var_stmt_next(parent);
			break;
		case EXPR_FIRSTS:
			node = _nt_prec_0();
			_nt_var_expr_next(parent);
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return node;
}

static void _nt_var_expr_next(ast_node_t **parent) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_nth_vardecl(parent);
			break;
		case TOK_SEMICOLON: CONSUME;
			break;
		default: _panic_expect(CONSUME, "\",\" or \";\"");
	}
}

static void _nt_var_stmt_next(ast_node_t **parent) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_nth_vardecl(parent);
			break;
		case TOK_KW_END:
		case TOK_KW_ELIF:
		case TOK_KW_ELSE:
		case TOK_EOF:
		case TOK_KW_VAR:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			break;
		default: _panic_expect(CONSUME, "\",\" or \"end\" or EOF or block member");
	}
}

static ast_node_t *_nt_type(void) {
	switch(PEEK) {
		case TOK_TYPE_NAT:
		case TOK_TYPE_INT:
		case TOK_TYPE_BOOL:
			return ast_pnode_new(&ps.ast, AST_TYPE, CONSUME->content);
		default: _panic_expect(CONSUME, "\"nat\" or \"int\" or \"bool\"");
	}
	return NULL;
}

static ast_node_t *_nt_stmt_common(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_IF: ;
			ast_node_t *branch = ast_pnode_new(&ps.ast, AST_IF_CASE, CONSUME->content);
			ast_pnode_left(branch, _nt_delim_stmt_expr());
			_expect(TOK_COLON);
			ast_pnode_right(branch, _nt_inner_stmt_expr());
			node = ast_lnode_new(&ps.ast, 4, AST_IF_LIST, EMPTY_STRING);
			node = ast_lnode_add(&ps.ast, node, branch);
			_nt_else(&node);
			_expect(TOK_KW_END);
			break;
		case TOK_KW_WHILE:
			node = ast_pnode_new(&ps.ast, AST_WHILE, CONSUME->content);
			ast_pnode_left(node, _nt_delim_stmt_expr());
			_expect(TOK_COLON);
			ast_pnode_right(node, _nt_inner_stmt_expr());
			_expect(TOK_KW_END);
			break;
		default: _panic_expect(CONSUME, "\"if\" or \"while\"");
	}
	return node;
}

static void _nt_else(ast_node_t **parent) {
	ast_node_t * branch = NULL;
loop:
	switch(PEEK) {
		case TOK_KW_ELIF: ;
			branch = ast_pnode_new(&ps.ast, AST_IF_CASE, CONSUME->content);
			ast_pnode_left(branch, _nt_delim_stmt_expr());
			_expect(TOK_COLON);
			ast_pnode_right(branch, _nt_inner_stmt_expr());
			*parent = ast_lnode_add(&ps.ast, *parent, branch);
			goto loop;
		case TOK_KW_ELSE: ;
			branch = ast_pnode_new(&ps.ast, AST_IF_CASE, CONSUME->content);
			ast_pnode_left(branch, NULL);
			ast_pnode_right(branch, _nt_inner_stmt_expr());
			*parent = ast_lnode_add(&ps.ast, *parent, branch);
			break;
		case TOK_KW_END:
			break;
		default: _panic_expect(CONSUME, "\"else\" or \"end\"");
	}
}

static ast_node_t *_nt_outer_stmt(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_DO: ;
			string_t content = CONSUME->content;
			node = _nt_block();
			node->content = content;
			_expect(TOK_KW_END);
			break;
		case TOK_KW_RETURN:
			node = ast_pnode_new(&ps.ast, AST_RETURN, CONSUME->content);
			ast_pnode_left(node, _nt_outer_stmt_expr());
			ast_pnode_right(node, NULL);
			break;
		case TOK_KW_IF:
		case TOK_KW_WHILE:
			node = _nt_stmt_common();
			break;
		default: _panic_expect(CONSUME, "statement");
	}
	return node;
}

static ast_node_t *_nt_inner_stmt(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_DO: ;
			string_t content = CONSUME->content;
			node = _nt_block();
			node->content = content;
			break;
		case TOK_KW_RETURN:
			node = ast_pnode_new(&ps.ast, AST_RETURN, CONSUME->content);
			ast_pnode_left(node, _nt_inner_stmt_expr());
			ast_pnode_right(node, NULL);
			break;
		case TOK_KW_IF:
		case TOK_KW_WHILE:
			node = _nt_stmt_common();
			break;
		default: _panic_expect(CONSUME, "statement");
	}
	return node;
}

static ast_node_t *_nt_outer_stmt_expr(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			node = _nt_outer_stmt();
			break;
		case EXPR_FIRSTS: ;
			node = _nt_prec_0();
			_expect(TOK_SEMICOLON);
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return node;
}

static ast_node_t *_nt_delim_stmt_expr(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			node = _nt_outer_stmt();
			break;
		case EXPR_FIRSTS:
			node = _nt_prec_0();
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return node;
}

static ast_node_t *_nt_inner_stmt_expr(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			node = _nt_inner_stmt();
			break;
		case EXPR_FIRSTS:
			node = _nt_prec_0();
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return node;
}

static ast_node_t *_nt_prec_0(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			node = _nt_prec_1();
			_nt_prec_0_();
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_0_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_ASSIGN:
		case TOK_OP_ASSIGN_ALT: CONSUME;
			_nt_prec_1();
			goto loop;
		case PREC_0_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
	return NULL;
}

static ast_node_t *_nt_prec_1(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			node = _nt_prec_2();
			_nt_prec_1_();
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_1_(void) {
loop:
	switch(PEEK) {
		case TOK_KW_AND: CONSUME;
			_nt_prec_2();
			goto loop;
		case TOK_KW_OR: CONSUME;
			_nt_prec_2();
			goto loop;
		case PREC_1_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
	return NULL; // Placeholder
}

static ast_node_t *_nt_prec_2(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS: ;
			node = _nt_unaries_2();
			if(node == NULL) node = _nt_prec_3();
			else ast_pnode_left(node, _nt_prec_3());
			_nt_prec_2_();
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_2_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_COMPARE: CONSUME;
			_nt_prec_3();
			goto loop;
		case PREC_2_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
	return NULL; // Placeholder
}

static ast_node_t *_nt_unaries_2(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_NOT:
			node = ast_pnode_new(&ps.ast, AST_OP_UNARY, CONSUME->content);
			break;
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_3(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			node = _nt_prec_4();
			_nt_prec_3_();
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_3_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_PLUS: CONSUME;
			_nt_prec_4();
			goto loop;
		case TOK_OP_MINUS: CONSUME;
			_nt_prec_4();
			goto loop;
		case PREC_3_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
	return NULL; // Placeholder
}

static ast_node_t *_nt_prec_4(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			node = _nt_prec_5();
			_nt_prec_4_();
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_4_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_MULT: CONSUME;
			_nt_prec_5();
			goto loop;
		case TOK_OP_DIV: CONSUME;
			_nt_prec_5();
			goto loop;
		case TOK_OP_MOD: CONSUME;
			_nt_prec_5();
			goto loop;
		case PREC_4_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
	return NULL; // Placeholder
}

static ast_node_t *_nt_prec_5(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS: ;
			node = _nt_unaries_5();
			if(node == NULL) node = _nt_term();
			else ast_pnode_left(node, _nt_term());
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_unaries_5(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
			node = ast_pnode_new(&ps.ast, AST_OP_UNARY, CONSUME->content);
			break;
		case TERM_FIRSTS:
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_term(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OPEN_ROUND: CONSUME;
			node = _nt_prec_0();
			_expect(TOK_CLOSE_ROUND);
			break;
		case TOK_IDENT:
			node = _nt_term_(ast_pnode_new(&ps.ast, AST_IDENT, CONSUME->content));
			break;
		case TOK_LIT_NUM:
		case TOK_KW_TRUE:
		case TOK_KW_FALSE:
		case TOK_KW_NIL:
			node = ast_pnode_new(&ps.ast, AST_LITERAL, CONSUME->content);
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_term_(ast_node_t *child) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OPEN_ROUND: CONSUME;
			node = _nt_func(child);
			_expect(TOK_CLOSE_ROUND);
			break;
		case PREC_5_FOLLOWS:
			node = child;
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_func(ast_node_t *child) {
	ast_node_t *node = ast_lnode_new(&ps.ast, 4, AST_CALL, EMPTY_STRING);
	node = ast_lnode_add(&ps.ast, node, child);
	switch(PEEK) {
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			node = ast_lnode_add(&ps.ast, node, _nt_delim_stmt_expr());
			_nt_func_(&node);
			break;
		case TOK_CLOSE_ROUND:
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static void _nt_func_(ast_node_t **parent) {
loop:
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			*parent = ast_lnode_add(&ps.ast, *parent, _nt_delim_stmt_expr());
			goto loop;
		case TOK_CLOSE_ROUND:
			break;
		default: _panic(CONSUME);
	}
}

// External Functions //

void parser_start(void) {
	ps.ast = ast_tree_new();
	ast_node_t *root = NULL;
	switch(PEEK) {
		case TOK_KW_VAR:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			root = _nt_block();
			_expect(TOK_EOF);
		case TOK_EOF:
			break;
		default: _panic_expect(CONSUME, "\"var\" statement or expression or EOF");
	}
	if(root == NULL) printf("The file is empty.\n");
	else ast_tree_visualize(root);
	ast_tree_free(&ps.ast);
}
