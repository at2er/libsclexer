#include "sclexer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int issymbol(int c);
static void read_empty(struct sclexer_tok *result);
static int read_int(struct sclexer_tok *result, struct sclexer *lexer);
static int read_int_neg(struct sclexer_tok *result, struct sclexer *lexer);
static void read_space(struct sclexer_tok *result, struct sclexer *lexer);
static void read_str(struct sclexer_tok *result, struct sclexer *lexer);
static void read_symbol(struct sclexer_tok *result, struct sclexer *lexer);

int issymbol(int c)
{
	char *result = strchr("~!@#$%^&*()-_=+[]{}:;'\"\\|,<.>/?", c);
	if (result)
		return *result;
	return 0;
}

void read_empty(struct sclexer_tok *result)
{
	result->type = SCLEXER_TOK_TYPE_EMPTY;
	result->type_data.c = '\0';
}

int read_int(struct sclexer_tok *result, struct sclexer *lexer)
{
	result->type_data.uint = 0;
	while (isdigit(lexer->cur[0])) {
		result->type_data.uint *= 10;
		result->type_data.uint += lexer->cur[0] - '0';
		sclexer_next_column(lexer);
	}
	if (!isspace(lexer->cur[0]) && !issymbol(lexer->cur[0]))
		return 1;
	result->type = SCLEXER_TOK_TYPE_INT;
	return 0;
}

int read_int_neg(struct sclexer_tok *result, struct sclexer *lexer)
{
	if (!isdigit(sclexer_next_column(lexer)))
		return 1;
	if (read_int(result, lexer))
		return 1;
	result->type_data.sint = -result->type_data.uint;
	result->type = SCLEXER_TOK_TYPE_INT_NEG;
	return 0;
}

void read_space(struct sclexer_tok *result, struct sclexer *lexer)
{
	result->type_data.c = lexer->cur[0];
	result->type = SCLEXER_TOK_TYPE_SPACE;
	while (isspace(lexer->cur[0]))
		sclexer_next_column(lexer);
}

void read_str(struct sclexer_tok *result, struct sclexer *lexer)
{
	char c;
	result->type_data.str.s = lexer->cur;
	result->type_data.str.len = 1;
	while (!isspace((c = sclexer_next_column(lexer)))) {
		if (SCLEXER_IS_STOP_CHRS(lexer->stop_chrs_in_str, c))
			break;
		result->type_data.str.len++;
	}
	result->type = SCLEXER_TOK_TYPE_STR;
}

void read_symbol(struct sclexer_tok *result, struct sclexer *lexer)
{
	result->type = SCLEXER_TOK_TYPE_SYM;
	result->type_data.c = lexer->cur[0];
	sclexer_next_column(lexer);
}

void sclexer_end(struct sclexer *lexer)
{
	fclose(lexer->fp);
}

int sclexer_get_line(struct sclexer *lexer)
{
	if (fgets(lexer->buf, BUFSIZ, lexer->fp) == NULL)
		return EOF;
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

char sclexer_next_column(struct sclexer *lexer)
{
	lexer->column++;
	lexer->cur = &lexer->cur[1];
	return lexer->cur[0];
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
		sclexer_next_column(lexer);
	}
	sclexer_next_column(lexer);
	return 0;
}

int sclexer_read_tok(struct sclexer_tok *result, struct sclexer *lexer)
{
#define end_with(COND, FUNC) \
	if (COND(lexer->cur[0])) { \
		FUNC(result, lexer); \
		goto end; \
	}
#define end_with_res(COND, FUNC) \
	if (COND(lexer->cur[0])) \
		return FUNC(result, lexer);

	switch (lexer->cur[0]) {
	case '\0': read_empty(result); goto end;
	case '\n':
		result->type = SCLEXER_TOK_TYPE_SPACE;
		result->type_data.c = '\n';
		goto end;
	case '-':
		if (isdigit(lexer->cur[1]))
			return read_int_neg(result, lexer);
	}
	end_with(isalpha,  read_str);
	end_with(isspace,  read_space);
	end_with(issymbol, read_symbol);
	end_with_res(isdigit, read_int);

#undef end_with
#undef end_with_res
end:
	return 0;
}

int sclexer_read_while(struct sclexer_str_slice *result,
		struct sclexer *lexer,
		int (*cond)(struct sclexer_str_slice *result,
			struct sclexer *lexer))
{
	int ret = 0;
	result->s = lexer->cur;
	result->len = 0;
	while ((ret = cond(result, lexer))) {
		if (ret == 2)
			return 1;
		if (lexer->cur[0] == '\0')
			return 1;
		result->len++;
		sclexer_next_column(lexer);
	}
	return 0;
}

long sclexer_record(struct sclexer *lexer, struct sclexer *recorder)
{
	if (!lexer || !recorder)
		return -1;
	*recorder = *lexer;
	return ftell(lexer->fp);
}

void sclexer_restore(struct sclexer *lexer, struct sclexer *recorder, long fpos)
{
	*lexer = *recorder;
	fseek(lexer->fp, fpos, SEEK_SET);
}
