#include "sclexer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char column_next(struct sclexer_context *context);
static int issymbol(int c);
static int read_empty(struct sclexer_tok *result);
static int read_int(struct sclexer_tok *result,
		struct sclexer_context *context);
static int read_int_neg(struct sclexer_tok *result,
		struct sclexer_context *context);
static int read_space(struct sclexer_tok *result,
		struct sclexer_context *context);
static int read_str(struct sclexer_tok *result,
		struct sclexer_context *context);
static int read_symbol(struct sclexer_tok *result,
		struct sclexer_context *context);

char column_next(struct sclexer_context *context)
{
	context->column++;
	context->cur = &context->cur[1];
	return context->cur[0];
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

int read_int(struct sclexer_tok *result, struct sclexer_context *context)
{
	result->kind_data.uint = 0;
	while (isdigit(context->cur[0])) {
		result->kind_data.uint *= 10;
		result->kind_data.uint += context->cur[0] - '0';
		column_next(context);
	}
	if (!isspace(context->cur[0]) && !issymbol(context->cur[0]))
		return 1;
	result->kind = SCLEXER_TOK_KIND_INT;
	return 0;
}

int read_int_neg(struct sclexer_tok *result, struct sclexer_context *context)
{
	if (!isdigit(column_next(context)))
		return 1;
	if (read_int(result, context))
		return 1;
	result->kind_data.sint = -result->kind_data.uint;
	result->kind = SCLEXER_TOK_KIND_INT_NEG;
	return 0;
}

int read_space(struct sclexer_tok *result, struct sclexer_context *context)
{
	while (isspace(context->cur[0]))
		column_next(context);
	result->kind_data.c = context->cur[0];
	result->kind = SCLEXER_TOK_KIND_SPACE;
	return 0;
}

int read_str(struct sclexer_tok *result, struct sclexer_context *context)
{
	result->kind_data.str.s = context->cur;
	result->kind_data.str.len = 1;
	while (!isspace(column_next(context)))
		result->kind_data.str.len++;
	result->kind = SCLEXER_TOK_KIND_STR;
	return 0;
}

int read_symbol(struct sclexer_tok *result, struct sclexer_context *context)
{
	result->kind = SCLEXER_TOK_KIND_SYM;
	result->kind_data.c = context->cur[0];
	column_next(context);
	return 0;
}

void sclexer_end(struct sclexer_context *context)
{
	fclose(context->fp);
}

int sclexer_get_line(struct sclexer_context *context)
{
	if (fgets(context->buf, BUFSIZ, context->fp) == NULL)
		return 1;
	context->cur = context->buf;
	context->line++;
	context->column = 1;
	return 0;
}

int sclexer_init(const char *fpath, struct sclexer_context *context)
{
	context->line = 0;
	context->column = 1;
	context->cur = context->buf;
	context->fp = fopen(fpath, "r");
	if (!context->fp)
		return 1;
	context->fpath = fpath;
	return 0;
}

int sclexer_read_to(struct sclexer_str_slice *result,
		struct sclexer_context *context,
		const char *s)
{
	result->s = context->cur;
	result->len = 1;
	while (strchr(s, context->cur[0]) == NULL) {
		if (context->cur[0] == '\0')
			return 1;
		result->len++;
		column_next(context);
	}
	column_next(context);
	return 0;
}

int sclexer_read_tok(struct sclexer_tok *result,
		struct sclexer_context *context)
{
	if (context->cur[0] == '\0')
		return read_empty(result);
	if (isdigit(context->cur[0]))
		return read_int(result, context);
	if (isalpha(context->cur[0]))
		return read_str(result, context);
	if (isspace(context->cur[0]))
		return read_space(result, context);
	if (issymbol(context->cur[0]))
		return read_symbol(result, context);
	if (context->cur[0] == '-' && isdigit(context->cur[1]))
		return read_int_neg(result, context);
	return 0;
}

int sclexer_read_while(struct sclexer_str_slice *result,
		struct sclexer_context *context,
		int (*cond)(struct sclexer_str_slice *cur,
			struct sclexer_context *context))
{
	result->s = context->cur;
	result->len = 1;
	while (cond(result, context)) {
		if (context->cur[0] == '\0')
			return 1;
		result->len++;
		column_next(context);
	}
	return 0;
}
