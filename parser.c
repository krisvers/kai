#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned long long int djb_hash(char * str, unsigned long long int len) {
	unsigned long long int hash = 5381;
	unsigned long long int i = 0;

	while (i < len) {
		hash = ((hash << 5) + hash) + str[i];
		++i;
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
} parser_key_t;

struct {
	parser_key_t * keyarray;
	parser_syntax_enum * valuearray;
	unsigned long long int length;
	is_char_valid_f isdelim;
} static parser_hash_table;

void printword(FILE * fp, char * start, unsigned long long int length) {
	if (start == NULL || length == 0) {
		fprintf(fp, "(nil)");
		return;
	}
	unsigned long long int i = 0;
	do {
		putc(start[i], fp);
	} while (++i < length);
}

int parser_hash_table_add(char * key, unsigned long long int keylen, unsigned long long int value) {
	unsigned long long int hash = djb_hash(key, keylen);
	if (parser_hash_table.keyarray == NULL) {
		parser_hash_table.length = 1;
		parser_hash_table.keyarray = malloc(parser_hash_table.length * sizeof(parser_key_t));
		if (parser_hash_table.keyarray == NULL) {
			abort();
		}
		parser_hash_table.valuearray = malloc(parser_hash_table.length * sizeof(parser_syntax_enum));
		if (parser_hash_table.valuearray == NULL) {
			abort();
		}

		parser_hash_table.keyarray[parser_hash_table.length - 1].hash = hash;
		parser_hash_table.valuearray[parser_hash_table.length - 1] = value;
		return 0;
	}

	++parser_hash_table.length;
	parser_hash_table.keyarray = realloc(parser_hash_table.keyarray, parser_hash_table.length * sizeof(parser_key_t));
	if (parser_hash_table.keyarray == NULL) {
		abort();
	}
	parser_hash_table.valuearray = realloc(parser_hash_table.valuearray, parser_hash_table.length * sizeof(parser_syntax_enum));
	if (parser_hash_table.valuearray == NULL) {
		abort();
	}

	parser_hash_table.keyarray[parser_hash_table.length - 1].hash = hash;
	parser_hash_table.valuearray[parser_hash_table.length - 1] = value;

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
		unsigned long long int hash = djb_hash(start, length);
		if (parser_hash_table.keyarray[i].hash == hash) {
			return parser_hash_table.valuearray[i];
		}
	}

	return PARSER_SYNTAX_UNKNOWN;
}

int parse(char * start, char ** out_end) {
	char c = 127;
	char nextc = 127;
	char prevc = 127;
	char lastvalidc = 127;
	unsigned long long int i = 0;
	unsigned long long int line = 1;

	struct {
		char * start;
		unsigned long long int length;
		parser_syntax_enum syntax;
	} word = {
		.start = NULL,
		.length = 0,
		.syntax = PARSER_SYNTAX_UNSET,
	};

	struct {
		enum {
			CONTEXT_STATE_NONE = 0,
			CONTEXT_STATE_VAR,
			CONTEXT_STATE_FUNC,
			CONTEXT_STATE_TYPED_ITEM,
		} state;
		unsigned char is_named;
		unsigned char is_typed;
		unsigned char is_valued;
		unsigned char is_done;
		struct {
			char * start;
			unsigned long long int length;
		} name;
		struct {
			char * start;
			unsigned long long int length;
		} type;
		struct {
			char * start;
			unsigned long long int length;
		} value;
	} context = {
		.state = CONTEXT_STATE_NONE,
		.is_named = 0,
		.is_typed = 0,
		.is_valued = 0,
		.is_done = 0,
		.name = {
			.start = NULL,
			.length = 0,
		},
		.type = {
			.start = NULL,
			.length = 0,
		},
		.value = {
			.start = NULL,
			.length = 0,
		},
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

		if (c == ':') {
			context.state = CONTEXT_STATE_TYPED_ITEM;
			context.is_named = 1;
			context.name.start = word.start;
			context.name.length = word.length;

			word.start = NULL;
			word.length = 0;
			word.syntax = PARSER_SYNTAX_UNSET;
			goto next_parse;
		}
		
		if (c == '=') {
			if (lastvalidc == ':') {
				context.is_typed = 0;
				context.is_valued = 1;
				context.type.start = NULL;
				context.type.length = 0;
				word.start = NULL;
				word.length = 0;
				goto next_parse;
			}
			word.syntax = parser_find_match(word.start, word.length);
			if (word.syntax == PARSER_SYNTAX_UNKNOWN && word.length > 0) {
				printf("Unknown syntax at (line: %llu, column: %llu) [", line, i);
				printword(stdout, word.start, word.length);
				puts("]");
				*out_end = &start[i];
				return 1;
			}
			context.is_typed = 1;
			context.is_valued = 1;
			context.type.start = word.start;
			context.type.length = word.length;
			word.start = NULL;
			word.length = 0;
			goto next_parse;
		}

		if (c == ';') {
			if (context.is_valued) {
				context.value.start = word.start;
				context.value.length = word.length;
			}
			printword(stdout, context.name.start, context.name.length);
			printf(":");
			printword(stdout, context.type.start, context.type.length);
			printf("=");
			printword(stdout, context.value.start, context.value.length);
			puts(";");
			context.is_done = 1;
			word.start = NULL;
			word.length = 0;
			goto next_parse;
		}

		if (isnewline(c)) {
			++line;
			goto next_parse;
		}

		if (iswhitespace(c)) {
			goto next_parse;
		}

		if (word.start == NULL) {
			word.start = &start[i];
			word.length = 0;
		}
		c = start[i];
		++word.length;

	next_parse:
		++i;
	}

	*out_end = &start[i];
	return 0;
}
