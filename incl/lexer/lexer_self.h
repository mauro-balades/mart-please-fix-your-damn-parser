#include "lexer.h"
#include <stdint.h>

/*
	This is the self-include for lexer.c which holds the
	constants for the keyword hash map. This header is not
	meant to be included in other places, hence the lack of
	a header guard. Use lexer.h directly.
*/

// Must be a power of two
#define MAP_SIZE 16

static const uint8_t map_sbox[256] = {
	0xc9, 0x80, 0x4f, 0x27, 0x57, 0x31, 0xbf, 0xd7, 0x5d, 0x96, 0x12, 0x58, 0x67, 0xf8, 0x3d, 0x60,
	0x75, 0x24, 0x65, 0xbb, 0x06, 0xad, 0x3c, 0x1f, 0x1c, 0x16, 0xf3, 0x5f, 0xbd, 0x4e, 0x7f, 0x18,
	0x3a, 0xb9, 0x76, 0xbe, 0x51, 0x69, 0x9e, 0xe1, 0x1a, 0x8b, 0x19, 0x15, 0xda, 0x84, 0xa0, 0x28,
	0xfa, 0x78, 0xfc, 0x33, 0xc7, 0xfb, 0xa9, 0x39, 0x4c, 0xa2, 0x81, 0x5e, 0x2a, 0x83, 0x22, 0xa1,
	0xa4, 0x77, 0x90, 0x0b, 0x04, 0x07, 0x63, 0x85, 0xd4, 0x62, 0x53, 0xe6, 0xc4, 0x72, 0xae, 0x35,
	0xab, 0xf0, 0x41, 0x66, 0xdc, 0xd8, 0x91, 0x0c, 0x86, 0xf1, 0x10, 0x8a, 0x26, 0xef, 0x6c, 0xaa,
	0x7d, 0x8f, 0xd1, 0x4b, 0xb7, 0xb4, 0xf5, 0x47, 0x2f, 0xd5, 0x99, 0xf9, 0x82, 0xd3, 0xb8, 0xc0,
	0x08, 0x40, 0x6a, 0xa5, 0x50, 0x49, 0xb1, 0xb3, 0x0d, 0x3f, 0xf6, 0x8e, 0xfd, 0xce, 0x79, 0x0f,
	0xb5, 0xd2, 0xd0, 0x52, 0xe0, 0xa3, 0x11, 0xf4, 0xeb, 0x56, 0xf7, 0x93, 0xdb, 0x23, 0x5a, 0xde,
	0x7e, 0x2e, 0xe8, 0xd6, 0xee, 0xbc, 0xca, 0x61, 0x20, 0x7c, 0x74, 0x2d, 0xe5, 0x03, 0x6d, 0x9a,
	0xac, 0x05, 0x64, 0x9c, 0x0a, 0x95, 0x3e, 0x01, 0xc1, 0x70, 0x6f, 0x7a, 0xec, 0x6e, 0xcd, 0x42,
	0xc8, 0x2c, 0xc3, 0xff, 0x25, 0x29, 0x14, 0xe3, 0x17, 0x44, 0x88, 0x97, 0x00, 0x5c, 0x3b, 0x5b,
	0x4a, 0xaf, 0xe7, 0xed, 0x21, 0x6b, 0xf2, 0x34, 0x1e, 0x59, 0x45, 0x89, 0x71, 0xc5, 0x9b, 0xd9,
	0xb0, 0x9d, 0xe2, 0x8d, 0x30, 0x09, 0x1d, 0x4d, 0xba, 0xcf, 0x36, 0xa7, 0x37, 0x43, 0xfe, 0x68,
	0xdf, 0x92, 0x98, 0xdd, 0x87, 0x8c, 0x1b, 0x7b, 0x38, 0x73, 0x55, 0x48, 0x2b, 0xa6, 0xb6, 0xb2,
	0x32, 0x46, 0xc2, 0xcb, 0xe9, 0xc6, 0xcc, 0xa8, 0x02, 0x54, 0x0e, 0x94, 0x9f, 0xea, 0x13, 0xe4
};

static const char *map_keys[MAP_SIZE] = {
	"true", "end", "not", "or", "false", "while", "nil", "nat",
	"bool", "", "do", "", "and", "int", "return", "if"
};

static const tok_type map_vals[MAP_SIZE] = {
	TOK_KW_TRUE, TOK_KW_END, TOK_KW_NOT, TOK_KW_OR, TOK_KW_FALSE, TOK_KW_WHILE, TOK_KW_NIL, TOK_TYPE_NAT,
	TOK_TYPE_BOOL, TOK_IDENT, TOK_KW_DO, TOK_IDENT, TOK_KW_AND, TOK_TYPE_INT, TOK_KW_RETURN, TOK_KW_IF
};
