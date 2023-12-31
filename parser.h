#ifndef KRISVERS_KAI_PARSER_H
#define KRISVERS_KAI_PARSER_H

typedef unsigned char (* is_char_valid_f)(char c);

typedef enum ParserSyntaxEnum {
	PARSER_SYNTAX_UNSET = -1,
	PARSER_SYNTAX_UNKNOWN = 0,
	PARSER_SYNTAX_SCOPE_START = 1,
	PARSER_SYNTAX_BRACKET_START,
	PARSER_SYNTAX_STRING_START,
	PARSER_SYNTAX_PARENTHESIS_START,
	PARSER_SYNTAX_SCOPE_DELIMITER,
	PARSER_SYNTAX_BRACKET_DELIMITER,
	PARSER_SYNTAX_STATEMENT_DELIMITER,
	PARSER_SYNTAX_STRING_DELIMITER,
	PARSER_SYNTAX_TYPE_DELIMITER,
	PARSER_SYNTAX_PARENTHESIS_DELIMITER,
	PARSER_SYNTAX_ASSIGNMENT,
	PARSER_SYNTAX_OPERATOR_ADD,
	PARSER_SYNTAX_OPERATOR_SUB,
	PARSER_SYNTAX_OPERATOR_MUL,
	PARSER_SYNTAX_OPERATOR_DIV,
	PARSER_SYNTAX_CHARACTER_LITERAL_SPECIFIER,
	PARSER_SYNTAX_LITERAL_ESCAPE,
	PARSER_SYNTAX_TYPE_U8,
	PARSER_SYNTAX_TYPE_U16,
	PARSER_SYNTAX_TYPE_U32,
	PARSER_SYNTAX_TYPE_U64,
	PARSER_SYNTAX_TYPE_S8,
	PARSER_SYNTAX_TYPE_S16,
	PARSER_SYNTAX_TYPE_S32,
	PARSER_SYNTAX_TYPE_S64,
	PARSER_SYNTAX_TYPE_F32,
	PARSER_SYNTAX_TYPE_F64,
	PARSER_SYNTAX_TYPE_STR,
	PARSER_SYNTAX_TYPE_PTR,
	PARSER_SYNTAX_NAME,
} parser_syntax_enum;

unsigned long long int djb_hash(char * str, unsigned long long int len);
unsigned long long int djb_hash_delimiter(char * str, char delimiter);
unsigned long long int djb_hash_injection(char * str, is_char_valid_f isvalid);

int parse(char * start, char ** out_end);
int parser_init(void);
int parser_deinit(void);

#endif
