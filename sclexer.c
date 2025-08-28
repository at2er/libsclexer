#include "sclexer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char column_next(struct sclexer *lexer);
static int issymbol(int c);
static int read_empty(struct sclexer_tok *result);
static int read_int(struct sclexer_tok *result, struct sclexer *lexer);
static int read_int_neg(struct sclexer_tok *result, struct sclexer *lexer);
static int read_space(struct sclexer_tok *result, struct sclexer *lexer);
static int read_str(struct sclexer_tok *result, struct sclexer *lexer);
static int read_symbol(struct sclexer_tok *result, struct sclexer *lexer);

char column_next(struct sclexer *lexer)
{
	lexer->column++;
	lexer->cur = &lexer->cur[1];
	return lexer->cur[0];
}

int issymbol(int c)
{
	char *result = strchr("~!#$%^&*()-_=+[]:;'\"\\,<.>/?", c);
	if (result)
		return *result;
	return 0;
}

int read_empty(struct sclexer_tok *result)
{
	result->kind = SCLEXER_TOK_KIND_EMPTY;
	result->kind_data.c = '\0';
	return 0;
}

int read_int(struct sclexer_tok *result, struct sclexer *lexer)
{
	result->kind_data.uint = 0;
	while (isdigit(lexer->cur[0])) {
		result->kind_data.uint *= 10;
		result->kind_data.uint += lexer->cur[0] - '0';
		column_next(lexer);
	}
	if (!isspace(lexer->cur[0]) && !issymbol(lexer->cur[0]))
		return 1;
	result->kind = SCLEXER_TOK_KIND_INT;
	return 0;
}

int read_int_neg(struct sclexer_tok *result, struct sclexer *lexer)
{
	if (!isdigit(column_next(lexer)))
		return 1;
	if (read_int(result, lexer))
		return 1;
	result->kind_data.sint = -result->kind_data.uint;
	result->kind = SCLEXER_TOK_KIND_INT_NEG;
	return 0;
}

int read_space(struct sclexer_tok *result, struct sclexer *lexer)
{
	while (isspace(lexer->cur[0]))
		column_next(lexer);
	result->kind_data.c = lexer->cur[0];
	result->kind = SCLEXER_TOK_KIND_SPACE;
	return 0;
}

int read_str(struct sclexer_tok *result, struct sclexer *lexer)
{
	result->kind_data.str.s = lexer->cur;
	result->kind_data.str.len = 1;
	while (!isspace(column_next(lexer)))
		result->kind_data.str.len++;
	result->kind = SCLEXER_TOK_KIND_STR;
	return 0;
}

int read_symbol(struct sclexer_tok *result, struct sclexer *lexer)
{
	result->kind = SCLEXER_TOK_KIND_SYM;
	result->kind_data.c = lexer->cur[0];
	column_next(lexer);
	return 0;
}

void sclexer_end(struct sclexer *lexer)
{
	fclose(lexer->fp);
}

int sclexer_get_line(struct sclexer *lexer)
{
	if (fgets(lexer->buf, BUFSIZ, lexer->fp) == NULL)
		return 1;
	lexer->cur = lexer->buf;
	lexer->line++;
	lexer->column = 1;
	return 0;
}

int sclexer_init(const char *fpath, struct sclexer *lexer)
{
	lexer->line = 0;
	lexer->column = 1;
	lexer->cur = lexer->buf;
	lexer->fp = fopen(fpath, "r");
	if (!lexer->fp)
		return 1;
	lexer->fpath = fpath;
	return 0;
}

int sclexer_read_to(struct sclexer_str_slice *result,
		struct sclexer *lexer,
		const char *s)
{
	result->s = lexer->cur;
	result->len = 1;
	while (strchr(s, lexer->cur[0]) == NULL) {
		if (lexer->cur[0] == '\0')
			return 1;
		result->len++;
		column_next(lexer);
	}
	column_next(lexer);
	return 0;
}

int sclexer_read_tok(struct sclexer_tok *result, struct sclexer *lexer)
{
	if (lexer->cur[0] == '\0')
		return read_empty(result);
	if (isdigit(lexer->cur[0]))
		return read_int(result, lexer);
	if (isalpha(lexer->cur[0]))
		return read_str(result, lexer);
	if (isspace(lexer->cur[0]))
		return read_space(result, lexer);
	if (issymbol(lexer->cur[0]))
		return read_symbol(result, lexer);
	if (lexer->cur[0] == '-' && isdigit(lexer->cur[1]))
		return read_int_neg(result, lexer);
	return 0;
}

int sclexer_read_while(struct sclexer_str_slice *result,
		struct sclexer *lexer,
		int (*cond)(struct sclexer_str_slice *cur,
			struct sclexer *lexer))
{
	result->s = lexer->cur;
	result->len = 1;
	while (cond(result, lexer)) {
		if (lexer->cur[0] == '\0')
			return 1;
		result->len++;
		column_next(lexer);
	}
	return 0;
}
