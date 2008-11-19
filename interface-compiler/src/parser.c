#if defined(WIN32)
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "common.h"

#include "parser.h"
#include "types.h"

#include <ctype.h>

#define MAX_LEXEME_SIZE 100

#if defined(WIN32)
	#define strdup _strdup
#endif

enum lexeme_type
{
	LEXEME_START,
	LEXEME_STOP,
	LEXEME_ID,
	LEXEME_OPEN_BRACE,
	LEXEME_CLOSE_BRACE,
};

enum lexer_state
{
	STATE_START,
	STATE_IN_COMMENT,
	STATE_IN_ID,
};

struct parser_data
{
	enum lexeme_type lexeme_type;
	char lexeme[MAX_LEXEME_SIZE];
	int lexeme_len;
	int line;
	int pos;
	const char *filename;
	FILE *file;
};

enum params_type
{
	REQUEST_PARAMS,
	RESPONSE_PARAMS
};

static struct parser_data parser_ctx;

static const char *lexeme_type_to_string(enum lexeme_type lexeme_type)
{
	switch (lexeme_type)
	{
	case LEXEME_START: return "begin of file";
	case LEXEME_STOP: return "end of file";
	case LEXEME_OPEN_BRACE: return "open curly brace \"{\"";
	case LEXEME_CLOSE_BRACE: return "close curly brace \"{\"";
	case LEXEME_ID: return "identifier like [a-z][a-z0-9_]*";
	default: die("unknown lexeme type=%d passed to the lexeme_type_to_string()", (int) lexeme_type);
	}
	return NULL;
}

static void fail(const char *expected_str)
{
	die("unexpected lexeme [%s] found at the file [%s], line=%d, position=%d. Expected: %s.",
		parser_ctx.lexeme, parser_ctx.filename, parser_ctx.line, parser_ctx.pos, expected_str);
}

static int read_next_char()
{
	int ch;

	parser_ctx.pos++;
	ch = fgetc(parser_ctx.file);
	return ch;
}

static void unread_char(char ch)
{
	int c;

	c = ungetc(ch, parser_ctx.file);
	assert(c == (int) ch);
	parser_ctx.pos--;
}

static const char *copy_current_lexeme()
{
	const char *s;

	s = strdup(parser_ctx.lexeme);
	return s;
}

static void clear_lexeme()
{
	parser_ctx.lexeme[0] = '\0';
	parser_ctx.lexeme_len = 0;
}

static void finalize_lexeme(enum lexeme_type lexeme_type)
{
	parser_ctx.lexeme_type = lexeme_type;
	parser_ctx.lexeme[parser_ctx.lexeme_len] = '\0';
}

static void append_to_lexeme(char ch)
{
	if (parser_ctx.lexeme_len + 1 == MAX_LEXEME_SIZE)
	{
		finalize_lexeme(LEXEME_ID);
		die("too long lexeme=[%s...] found at the file [%s], line=%d, position=%d",
			parser_ctx.lexeme, parser_ctx.filename, parser_ctx.line, parser_ctx.pos);
	}
	parser_ctx.lexeme[parser_ctx.lexeme_len++] = ch;
}

static void inc_line_count()
{
	parser_ctx.line++;
	parser_ctx.pos = 0;
}

static void init_lexer()
{
	parser_ctx.lexeme_type = LEXEME_START;
	parser_ctx.lexeme[0] = '\0';
	parser_ctx.lexeme_len = 0;
	parser_ctx.line = 1;
	parser_ctx.pos = 0;
}

static int is_whitespace(char ch)
{
	int is_success;
	
	is_success = isspace(ch);
	return is_success;
}

static int is_id_first_char(char ch)
{
	int is_success;

	is_success = isalpha(ch);
	return is_success;
}

static int is_id_char(char ch)
{
	int is_success;

	is_success = (isalnum(ch) || ch == '_');
	return is_success;
}

static void read_next_lexeme()
{
	enum lexer_state state;

	clear_lexeme();
	state = STATE_START;
	for (;;)
	{
		int ch_int;
		char ch;

		ch_int = read_next_char();
		if (ch_int == EOF)
		{
			switch (state)
			{
			case STATE_START:
				finalize_lexeme(LEXEME_STOP);
				return;
			case STATE_IN_COMMENT:
				finalize_lexeme(LEXEME_STOP);
				return;
			case STATE_IN_ID:
				finalize_lexeme(LEXEME_ID);
				return;
			}
		}

		ch = (char) ch_int;
		switch (ch)
		{
		case '#':
			switch (state)
			{
			case STATE_START:
				state = STATE_IN_COMMENT;
				continue;
			case STATE_IN_COMMENT:
				continue;
			case STATE_IN_ID:
				unread_char(ch);
				finalize_lexeme(LEXEME_ID);
				return;
			}
		case '\n':
			inc_line_count();
			switch (state)
			{
			case STATE_START:
				continue;
			case STATE_IN_COMMENT:
				state = STATE_START;
				continue;
			case STATE_IN_ID:
				finalize_lexeme(LEXEME_ID);
				return;
			}
		case '{':
			switch (state)
			{
			case STATE_START:
				append_to_lexeme('{');
				finalize_lexeme(LEXEME_OPEN_BRACE);
				return;
			case STATE_IN_COMMENT:
				continue;
			case STATE_IN_ID:
				unread_char(ch);
				finalize_lexeme(LEXEME_ID);
				return;
			}
		case '}':
			switch (state)
			{
			case STATE_START:
				append_to_lexeme('}');
				finalize_lexeme(LEXEME_CLOSE_BRACE);
				return;
			case STATE_IN_COMMENT:
				continue;
			case STATE_IN_ID:
				unread_char(ch);
				finalize_lexeme(LEXEME_ID);
				return;
			}
		default:
			switch (state)
			{
			case STATE_START:
				if (is_whitespace(ch))
				{
					continue;
				}
				if (is_id_first_char(ch))
				{
					append_to_lexeme(ch);
					state = STATE_IN_ID;
					continue;
				}
				die("unexpected character=[%c] found at the file [%s], line=%d, position=%d. Expected: [{}\\na-z].",
					ch, parser_ctx.filename, parser_ctx.line, parser_ctx.pos);
			case STATE_IN_COMMENT:
				continue;
			case STATE_IN_ID:
				if (is_whitespace(ch))
				{
					finalize_lexeme(LEXEME_ID);
					return;
				}
				if (is_id_char(ch))
				{
					append_to_lexeme(ch);
					continue;
				}
				die("unexpected character=[%c] found at the file [%s], line=%d, position=%d. Expected: [{}\\na-z0-9_].",
					ch, parser_ctx.filename, parser_ctx.line, parser_ctx.pos);
			}
		}
	}
}

static int test(enum lexeme_type lexeme_type)
{
	return (parser_ctx.lexeme_type == lexeme_type);
}

static int test_id(const char *s)
{
	return (test(LEXEME_ID) && (strcmp(s, parser_ctx.lexeme) == 0));
}

static void match(enum lexeme_type lexeme_type)
{
	if (test(lexeme_type))
	{
		read_next_lexeme();
	}
	else
	{
		const char *s;

		s = lexeme_type_to_string(lexeme_type);
		fail(s);
	}
}

static void match_id(const char *s)
{
	if (test_id(s))
	{
		match(LEXEME_ID);
	}
	else
	{
		fail(s);
	}
}

static const struct param *match_param(enum params_type params_type)
{
	struct param *param;

	param = (struct param *) malloc(sizeof(*param));
	param->is_key = 0;
	if (params_type == REQUEST_PARAMS)
	{
		if (test_id("key"))
		{
			param->is_key = 1;
			match(LEXEME_ID);
		}
	}

	if (test_id("uint32"))
	{
		param->type = PARAM_UINT32;
	}
	else if (test_id("int32"))
	{
		param->type = PARAM_INT32;
	}
	else if (test_id("uint64"))
	{
		param->type = PARAM_UINT64;
	}
	else if (test_id("int64"))
	{
		param->type = PARAM_INT64;
	}
	else if (test_id("char_array"))
	{
		param->type = PARAM_CHAR_ARRAY;
	}
	else if (test_id("wchar_array"))
	{
		param->type = PARAM_WCHAR_ARRAY;
	}
	else if (test_id("blob"))
	{
		param->type = PARAM_BLOB;
	}
	else
	{
		fail("parameter type");
	}
	match(LEXEME_ID);

	param->name = copy_current_lexeme();
	match(LEXEME_ID);

	return param;
}

static const struct param_list *match_params(enum params_type params_type)
{
	struct param_list *first_entry = NULL;
	struct param_list *last_entry = NULL;
	struct param_list *entry;

	if (params_type == REQUEST_PARAMS)
	{
		match_id("request");
	}
	else
	{
		match_id("response");
	}
	match(LEXEME_OPEN_BRACE);
	while (!test(LEXEME_CLOSE_BRACE))
	{
		entry = (struct param_list *) malloc(sizeof(*entry));
		entry->next = NULL;
		entry->param = match_param(params_type);
		if (first_entry == NULL)
		{
			first_entry = last_entry = entry;
		}
		else
		{
			last_entry->next = entry;
			last_entry = entry;
		}
	}
	match(LEXEME_CLOSE_BRACE);

	return first_entry;
}

static const struct method *match_method()
{
	struct method *method;

	method = (struct method *) malloc(sizeof(*method));

	match_id("method");
	method->name = copy_current_lexeme();
	match(LEXEME_ID);
	match(LEXEME_OPEN_BRACE);
	method->request_params = match_params(REQUEST_PARAMS);
	method->response_params = match_params(RESPONSE_PARAMS);
	match(LEXEME_CLOSE_BRACE);

	return method;
}

static const struct method_list *match_methods()
{
	struct method_list *first_entry;
	struct method_list *last_entry;
	struct method_list *entry;

	entry = (struct method_list *) malloc(sizeof(*entry));
	entry->next = NULL;
	entry->method = match_method();
	first_entry = last_entry = entry;
	while (test_id("method"))
	{
		entry = (struct method_list *) malloc(sizeof(*entry));
		entry->next = NULL;
		entry->method = match_method();
		last_entry->next = entry;
		last_entry = entry;
	}

	return first_entry;
}

static const struct interface *match_interface()
{
	struct interface *interface;

	interface = (struct interface *) malloc(sizeof(*interface));

	match(LEXEME_START);
	match_id("interface");
	interface->name = copy_current_lexeme();
	match(LEXEME_ID);
	match(LEXEME_OPEN_BRACE);
	interface->methods = match_methods();
	match(LEXEME_CLOSE_BRACE);
	match(LEXEME_STOP);

	return interface;
}

const struct interface *parse_interface(const char *filename)
{
	const struct interface *interface;
	int rv;

	parser_ctx.filename = filename;
	parser_ctx.file = fopen(filename, "rt");
	if (parser_ctx.file == NULL)
	{
		die("cannot open the file [%s]. errno=%d", filename, errno);
	}
	init_lexer();
	interface = match_interface();
	rv = fclose(parser_ctx.file);
	assert(rv == 0);

	return interface;
}
