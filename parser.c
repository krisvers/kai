#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned long long int djb_hash(char * str, unsigned long long int len) {
	unsigned long long int hash = 5381;
	char c;

	while (len-- && str++) {
		c = *str;
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

unsigned long long int djb_hash_delimiter(char * str, char delimiter) {
	unsigned long long int hash = 5381;
	char c = *str;

	while (c != delimiter) {
		c = *(++str);
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

unsigned long long int djb_hash_injection(char * str, is_char_valid_f isdelim) {
	unsigned long long int hash = 5381;
	char c = 127;

	while (isdelim(c) && str++) {
		c = *str;
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

unsigned char isdelim_default(char c) {
	return (c == '\0' || c == ';');
}

typedef struct ParserKey {
	unsigned long long int hash;
	unsigned long long int length;
	char * original;
} parser_key_t;

struct {
	parser_key_t * keyarray;
	parser_syntax_enum * valuearray;
	unsigned long long int length;
	is_char_valid_f isdelim;
} static parser_hash_table;

int parser_hash_table_add(char * key, unsigned long long int keylen, unsigned long long int value) {
	unsigned long long int hash = djb_hash(key, keylen);
	if (parser_hash_table.keyarray == NULL) {
		parser_hash_table.length = 1;
		parser_hash_table.keyarray = malloc(parser_hash_table.length * sizeof(key_t));
		if (parser_hash_table.keyarray == NULL) {
			abort();
		}
		parser_hash_table.valuearray = malloc(parser_hash_table.length * sizeof(parser_syntax_enum));
		if (parser_hash_table.valuearray == NULL) {
			abort();
		}

		parser_hash_table.keyarray[0].hash = hash;
		parser_hash_table.keyarray[0].length = keylen;
		parser_hash_table.keyarray[0].original = strdup(key);
		parser_hash_table.valuearray[0] = value;
		return 0;
	}

	unsigned long long int i;
	for (i = 0; i < parser_hash_table.length; ++i) {
		if (parser_hash_table.keyarray[i].hash == hash) {
			break;
		}
	}

	/* matching key already in hash table */
	if (i < parser_hash_table.length) {
		return -1;
	}

	parser_hash_table.keyarray = realloc(parser_hash_table.keyarray, parser_hash_table.length * sizeof(key_t));
	if (parser_hash_table.keyarray == NULL) {
		abort();
	}
	parser_hash_table.valuearray = realloc(parser_hash_table.valuearray, parser_hash_table.length * sizeof(parser_syntax_enum));
	if (parser_hash_table.valuearray == NULL) {
		abort();
	}

	parser_hash_table.keyarray[0].hash = hash;
	parser_hash_table.keyarray[0].length = keylen;
	parser_hash_table.keyarray[0].original = strdup(key);
	parser_hash_table.valuearray[0] = value;

	return 0;
}

int parser_init(void) {
	parser_hash_table_add("{", 1, PARSER_SYNTAX_SCOPE_START);
	parser_hash_table_add("[", 1, PARSER_SYNTAX_BRACKET_START);
	parser_hash_table_add("\"", 1, PARSER_SYNTAX_STRING_START);
	parser_hash_table_add("(", 1, PARSER_SYNTAX_PARENTHESIS_START);
	parser_hash_table_add("}", 1, PARSER_SYNTAX_SCOPE_DELIMITER);
	parser_hash_table_add("]", 1, PARSER_SYNTAX_BRACKET_DELIMITER);
	parser_hash_table_add(")", 1, PARSER_SYNTAX_PARENTHESIS_DELIMITER);
	parser_hash_table_add("\"", 1, PARSER_SYNTAX_STRING_DELIMITER);
	parser_hash_table_add("{", 1, PARSER_SYNTAX_STATEMENT_DELIMITER);
	parser_hash_table_add(":", 1, PARSER_SYNTAX_TYPE_DELIMITER);
	parser_hash_table_add("=", 1, PARSER_SYNTAX_ASSIGNMENT);
	parser_hash_table_add("+", 1, PARSER_SYNTAX_OPERATOR_ADD);
	parser_hash_table_add("-", 1, PARSER_SYNTAX_OPERATOR_SUB);
	parser_hash_table_add("*", 1, PARSER_SYNTAX_OPERATOR_MUL);
	parser_hash_table_add("/", 1, PARSER_SYNTAX_OPERATOR_DIV);
	parser_hash_table_add("'", 1, PARSER_SYNTAX_CHARACTER_LITERAL_SPECIFIER);
	parser_hash_table_add("\\", 1, PARSER_SYNTAX_LITERAL_ESCAPE);
	parser_hash_table_add("u8", 2, PARSER_SYNTAX_TYPE_U8);
	parser_hash_table_add("u16", 3, PARSER_SYNTAX_TYPE_U16);
	parser_hash_table_add("u32", 3, PARSER_SYNTAX_TYPE_U32);
	parser_hash_table_add("u64", 3, PARSER_SYNTAX_TYPE_U64);
	parser_hash_table_add("s8", 2, PARSER_SYNTAX_TYPE_S8);
	parser_hash_table_add("s16", 3, PARSER_SYNTAX_TYPE_S16);
	parser_hash_table_add("s32", 3, PARSER_SYNTAX_TYPE_S32);
	parser_hash_table_add("s64", 3, PARSER_SYNTAX_TYPE_S64);

	return 0;
}

int parser_deinit(void) {
	if (parser_hash_table.keyarray != NULL) {
		for (unsigned long long int i = 0; i < parser_hash_table.length; ++i) {
			free(parser_hash_table.keyarray[i].original);
		}
		free(parser_hash_table.keyarray);
	}
	if (parser_hash_table.valuearray != NULL) {
		free(parser_hash_table.valuearray);
	}

	return 0;
}

unsigned char iswhitespace(char c) {
	return (c == '\t' || c == ' ');
}

unsigned char isnewline(char c) {
	return (c == '\n');
}

parser_syntax_enum parser_find_match(char * start, unsigned long long int length) {
	for (unsigned long long int i = 0; i < parser_hash_table.length; ++i) {
		if (parser_hash_table.keyarray[i].length == length) {
			if (strncmp(start, parser_hash_table.keyarray[i].original, length) == 0) {
				return parser_hash_table.valuearray[i];
			}
		}
	}

	return PARSER_SYNTAX_UNKNOWN;
}

int parse(char * start, char ** out_end) {
	char c = 127;
	char nextc = 127;
	char prevc = 127;
	unsigned long long int i = 0;
	unsigned long long int line = 0;

	struct {
		char * start;
		unsigned long long int length;
		parser_syntax_enum syntax;
	} word = {
		.start = NULL,
		.length = 0,
		.syntax = PARSER_SYNTAX_UNSET,
	};

	while (c != '\0') {
		c = start[i];
		if (c == '\r') {
			goto next_parse;
		}

		nextc = start[i + 1];
		if (i != 0) {
			prevc = start[i - 1];
		}
		
		if (iswhitespace(c)) {
			if (word.syntax == PARSER_SYNTAX_UNSET && word.length > 0) {
				printf("Unknown syntax at (line: %llu, column: %llu)\n", line, i);
				*out_end = &start[i];
				return 1;
			}
			word.start = NULL;
			word.length = 0;
			word.syntax = PARSER_SYNTAX_UNSET;
			goto next_parse;
		}

		if (isnewline(c)) {
			++line;
			goto next_parse;
		}

		if (word.start == NULL) {
			word.start = &start[i];
			word.length = 0;
		}
		++word.length;

		word.syntax = parser_find_match(word.start, word.length);
		if (word.syntax >= PARSER_SYNTAX_UNKNOWN) {
			printf("%*.s %i\n", (unsigned int) word.length, word.start, word.syntax);
		}

	next_parse:
		++i;
	}

	*out_end = &start[i];
	return 0;
}
