/**
 * SPDX-License-Identifier: MIT
 */
#include "sclexer.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TOKENS_CAPACITY 64
#define ERR_FMT "libsclexer: %s:%s:%d: "
#define ERR_FMT_ARG __FILE__, __func__, __LINE__

//#define ENABLE_MSG_COLOR
#ifdef ENABLE_MSG_COLOR
#define MSG_COLOR(MSG, COLOR) COLOR MSG MSG_COLOR_RESET
#define MSG_COLOR_LOC(MSG) MSG_COLOR(MSG, "\x1b[31m")
#define MSG_COLOR_RESET "\x1b[0m"
#else
#define MSG_COLOR(MSG, ...) MSG
#define MSG_COLOR_LOC(MSG) MSG
#define MSG_COLOR_RESET
#endif // ENABLE_MSG_COLOR

//#define ENABLE_MORE_LOC_MSG
#ifdef ENABLE_MORE_LOC_MSG
#define TOK_LOC_FMT MSG_COLOR_LOC("f:%s,l:%lu,c:%lu")
#define TOK_LOC_UNWRAP(TOK) (TOK)->loc.fpath, (TOK)->loc.line, (TOK)->loc.column
#else
#define TOK_LOC_FMT MSG_COLOR_LOC("l:%lu,c:%lu")
#define TOK_LOC_UNWRAP(TOK) (TOK)->loc.line, (TOK)->loc.column
#endif // ENABLE_MORE_LOC_MSG

#define TOK_KIND_FMT MSG_COLOR("%s", "\x1b[32m")
#define TOK_KIND_FMT_ARG(TOK) kind_names[(TOK)->kind]

/* For public functions from `sclexer.h` */
#define check(E) \
	if (!(E)) { \
		eprintf(ERR_FMT"failed to check '%s'\n", \
				ERR_FMT_ARG, #E); \
	}

/* https://github.com/Gottox/smu
 * See `LICENSE.smu` for the license. {{{
 */
static void eprintf(const char *format, ...);
static void *ereallocz(void *p, size_t siz);

void eprintf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void *ereallocz(void *p, size_t siz)
{
	void *r;
	if (p) {
		r = realloc(p, siz);
	} else {
		r = calloc(1, siz);
	}
	if (r)
		return r;
	eprintf(ERR_FMT"failed to malloc() %u bytes\n", ERR_FMT_ARG, siz);
	return NULL;
}
/* }}} */

static void advance(struct sclexer *self, size_t readed);
/**
 * @return: 0 on compare failed, otherwise compared string length.
 */
static size_t cmp_src_with_cstr(const char *cur, const char *cstr);
static size_t do_ident(struct sclexer *self, struct sclexer_tok *tok);
static void drop_space(struct sclexer *self);
static size_t drop_until_endl(struct sclexer *self);
static bool is_prev_eol(struct sclexer_tok *tokens,
		struct sclexer_tok *cur_tok,
		size_t count);
static void next_line(struct sclexer *self);
static size_t try_comment(struct sclexer *self);
static size_t try_digit(struct sclexer *self, struct sclexer_tok *tok);
static bool try_endl(struct sclexer *self, struct sclexer_tok *tok);
static bool try_indent(struct sclexer *self, struct sclexer_tok *tok);
static void try_keyword(struct sclexer *self, struct sclexer_tok *tok);
static size_t try_string(struct sclexer *self, struct sclexer_tok *tok);
static size_t try_symbol(struct sclexer *self, struct sclexer_tok *tok);

static const char *kind_names[SCLEXER_TOK_KIND_COUNT] = {
	[SCLEXER_UNKNOWN_TOK] = "UNKNOWN_TOK",
	[SCLEXER_EOF]         = "EOF",
	[SCLEXER_EOL]         = "EOL",
	[SCLEXER_IDENT]       = "IDENT",
	[SCLEXER_INT]         = "INT",
	[SCLEXER_INT_NEG]     = "INT_NEG",
	[SCLEXER_KEYWORD]     = "KEYWORD",
	[SCLEXER_STRING]      = "STRING",
	[SCLEXER_SYMBOL]      = "SYMBOL",

	[SCLEXER_INDENT_BLOCK_BEGIN] = "INDENT_BLOCK_BEGIN",
	[SCLEXER_INDENT_BLOCK_END]   = "INDENT_BLOCK_END"
};

void advance(struct sclexer *self, size_t readed)
{
	self->_cur = &self->_cur[readed];
	self->_loc.column += readed;
}

size_t cmp_src_with_cstr(const char *cur, const char *cstr)
{
	size_t i = 0;
	for (; cur[i] != '\0' && cstr[i] != '\0'; i++) {
		if (cur[i] != cstr[i])
			return 0;
	}
	return i;
}

size_t do_ident(struct sclexer *self, struct sclexer_tok *tok)
{
	size_t readed = 0;
	if (!self->is_ident(self->_cur[0], true))
		return 0;
	readed = 1;
	for (; self->_cur[readed] != '\0'; readed++) {
		if (!self->is_ident(self->_cur[readed], false))
			goto end;
	}
end:
	tok->kind = SCLEXER_IDENT;
	tok->src.len = readed;
	tok->data.str.begin = tok->src.begin;
	tok->data.str.len = readed;
	return readed;
}

void drop_space(struct sclexer *self)
{
	size_t readed = 0;
	if (self->_cur[readed] == '\n')
		return;
	while (isspace(self->_cur[readed]))
		readed++;
	advance(self, readed);
}

size_t drop_until_endl(struct sclexer *self)
{
	size_t readed = 0;
	for (; self->_cur[readed] != '\0'; readed++) {
		if (self->_cur[readed] == '\n')
			return readed + 1;
	}
	return 0;
}

bool is_prev_eol(struct sclexer_tok *tokens,
		struct sclexer_tok *cur_tok,
		size_t count)
{
	if (count == 0)
		return false;
	if (cur_tok->kind != SCLEXER_EOL)
		return false;
	if (tokens[count - 1].kind != SCLEXER_EOL)
		return false;
	return true;
}

void next_line(struct sclexer *self)
{
	self->_after_endl = true;
	self->_loc.line++;
	self->_loc.column = 1;
}

size_t try_comment(struct sclexer *self)
{
	for (size_t i = 0; i < self->comments_count; i++) {
		if (cmp_src_with_cstr(self->_cur, self->comments[i]))
			return drop_until_endl(self);
	}
	return 0;
}

size_t try_digit(struct sclexer *self, struct sclexer_tok *tok)
{
	size_t readed = 0;
	tok->data.uint = 0;
	if (self->_cur[0] == '-') {
		if (!isdigit(self->_cur[1]))
			return 0;
		readed = 1;
	}
	if (!isdigit(self->_cur[0]))
		return 0;
	for (; isdigit(self->_cur[readed]); readed++) {
		tok->data.uint *= 10;
		tok->data.uint += self->_cur[readed] - '0';
	}
	tok->kind = SCLEXER_INT;
	if (self->_cur[0] == '-') {
		tok->data.sint = -(tok->data.uint);
		tok->kind = SCLEXER_INT_NEG;
	}
	return readed;
}

bool try_endl(struct sclexer *self, struct sclexer_tok *tok)
{
	size_t readed = 1;
	if (self->_cur[0] != '\n' && (readed = try_comment(self)) == 0) {
		return false;
	}
	tok->kind = SCLEXER_EOL;
	tok->src.len = readed;
	advance(self, readed);
	next_line(self);
	return true;
}

bool try_indent(struct sclexer *self, struct sclexer_tok *tok)
{
	size_t readed = 0;
	if (!self->enable_indent)
		return false;
	if (!self->_after_endl)
		return false;
	while (self->_cur[readed] == '\t')
		readed++;
	if (readed > self->_last_ident) {
		tok->kind = SCLEXER_INDENT_BLOCK_BEGIN;
	} else if (readed < self->_last_ident) {
		tok->kind = SCLEXER_INDENT_BLOCK_END;
	} else {
		return false;
	}
	self->_last_ident = readed;
	return true;
}

void try_keyword(struct sclexer *self, struct sclexer_tok *tok)
{
	for (size_t i = 0; i < self->keywords_count; i++) {
		if (strlen(self->keywords[i]) != tok->src.len)
			continue;
		if (strncmp(self->keywords[i],
					tok->src.begin,
					tok->src.len) == 0) {
			tok->data.keyword = i;
			tok->kind = SCLEXER_KEYWORD;
			return;
		}
	}
}

size_t try_string(struct sclexer *self, struct sclexer_tok *tok)
{
	size_t readed = 0;
	if (self->_cur[0] != '"')
		return 0;
	readed = 1;
	for (; self->_cur[readed] != '"'; readed++) {
		switch (self->_cur[readed]) {
		case '\0':
		case '\n':
			return 0;
		}
	}
	readed++;
	tok->data.str.begin = &self->_cur[1];
	tok->data.str.len = readed - 2;
	tok->kind = SCLEXER_STRING;
	return readed;
}

size_t try_symbol(struct sclexer *self, struct sclexer_tok *tok)
{
	size_t prev = 0, readed = 0;
	for (size_t i = 0; i < self->symbols_count; i++) {
		readed = cmp_src_with_cstr(self->_cur, self->symbols[i]);
		if (readed == 0)
			continue;
		if (readed < prev)
			continue;
		tok->data.symbol = i;
		tok->kind = SCLEXER_SYMBOL;
		prev = readed;
	}
	return prev;
}

bool sclexer_default_is_ident(char c, bool begin)
{
	if (begin && isdigit(c))
		return false;
	if (isalnum(c))
		return true;
	return false;
}

void sclexer_dup_tok(struct sclexer_tok *dst, struct sclexer_tok *src)
{
	check(dst && src);
	memcpy(dst, src, sizeof(*src));
}

bool sclexer_get_tok(struct sclexer *self,
		struct sclexer_tok *tok)
{
	size_t readed = 0;
	check(self && tok);
	check(self->src && self->_cur);

	tok->src.begin = self->_cur;
	tok->loc = self->_loc;
	tok->kind = SCLEXER_UNKNOWN_TOK;

	if (try_indent(self, tok)) {
		self->_after_endl = false;
		return true;
	}
	if (self->_after_endl)
		self->_after_endl = false;

	drop_space(self);
	if (self->_cur[0] == '\0') {
		tok->kind = SCLEXER_EOF;
		return false;
	}

	tok->src.begin = self->_cur;
	tok->loc = self->_loc;

	if (try_endl(self, tok))
		return true;
	if ((readed = try_digit(self, tok)))
		goto end;
	if ((readed = try_string(self, tok)))
		goto end;
	if ((readed = try_symbol(self, tok)))
		goto end;
	if ((readed = do_ident(self, tok))) {
		try_keyword(self, tok);
		goto end;
	}
	eprintf(ERR_FMT"unknown token '%c'\n", ERR_FMT_ARG, self->_cur[0]);
	return false;
end:
	advance(self, readed);
	tok->src.len = readed;
	return true;
}

void sclexer_init(struct sclexer *self, const char *fpath)
{
	check(self)
	check(self->src)
	check(fpath)
	if (!self->comments
			&& !self->symbols
			&& !self->keywords) {
		eprintf(ERR_FMT"need more parameters\n", ERR_FMT_ARG);
	}
	if (!self->is_ident)
		self->is_ident = sclexer_default_is_ident;
	self->_cur = self->src;
	self->_last_ident = 0;
	self->_loc.fpath  = fpath;
	self->_loc.line   = 1;
	self->_loc.column = 1;
}

const char *sclexer_kind_names(enum SCLEXER_TOK_KIND kind)
{
	if (kind >= SCLEXER_TOK_KIND_COUNT)
		return NULL;
	return kind_names[kind];
}

size_t sclexer_get_tokens(struct sclexer *self, struct sclexer_tok **result)
{
	size_t count = 0, capacity = DEFAULT_TOKENS_CAPACITY;
	struct sclexer_tok cur_tok = {0};
	struct sclexer_tok *tokens = NULL;
	tokens = ereallocz(tokens, sizeof(*tokens) * capacity);
	for (; sclexer_get_tok(self, &cur_tok); count++) {
		if (is_prev_eol(tokens, &cur_tok, count)) {
			count--;
			continue;
		}
		sclexer_dup_tok(&tokens[count], &cur_tok);
	}
	*result = tokens;
	return count;
}

/* shits, is's cannot be readed. */
void sclexer_print_tok(struct sclexer *self, struct sclexer_tok *tok)
{
	switch (tok->kind) {
	case SCLEXER_IDENT:
	case SCLEXER_STRING:
		printf(TOK_KIND_FMT"(len=%lu, '%.*s', "TOK_LOC_FMT")\n",
				TOK_KIND_FMT_ARG(tok),
				tok->data.str.len,
				(int)tok->data.str.len,
				tok->data.str.begin,
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_INT:
		printf(TOK_KIND_FMT"(%lu, "TOK_LOC_FMT")\n",
				TOK_KIND_FMT_ARG(tok),
				tok->data.uint,
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_INT_NEG:
		printf(TOK_KIND_FMT"(%ld, "TOK_LOC_FMT")\n",
				TOK_KIND_FMT_ARG(tok),
				tok->data.sint,
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_KEYWORD:
		printf(TOK_KIND_FMT"('%s', "TOK_LOC_FMT")\n",
				TOK_KIND_FMT_ARG(tok),
				self->keywords[tok->data.keyword],
				TOK_LOC_UNWRAP(tok));
		break;
	case SCLEXER_SYMBOL:
		printf(TOK_KIND_FMT"('%s', "TOK_LOC_FMT")\n",
				TOK_KIND_FMT_ARG(tok),
				self->symbols[tok->data.symbol],
				TOK_LOC_UNWRAP(tok));
		break;
	default:
		printf(TOK_KIND_FMT"("TOK_LOC_FMT")\n",
				TOK_KIND_FMT_ARG(tok),
				TOK_LOC_UNWRAP(tok));
		break;
	}
}

size_t sclexer_read_file(char **result, const char *fpath)
{
	/* https://github.com/Gottox/smu */
	char *buf = NULL;
	size_t bsiz, len = 0, readed = 0;
	FILE *fp;
	if (!(fp = fopen(fpath, "r")))
		eprintf(ERR_FMT"no such file or directory: %s\n",
				ERR_FMT_ARG, fpath);
	bsiz = 2 * BUFSIZ;
	buf = ereallocz(buf, bsiz);
	while ((readed = fread(buf + len, 1, BUFSIZ, fp))) {
		len += readed;
		if (BUFSIZ + len + 1 > bsiz) {
			bsiz += BUFSIZ;
			buf = ereallocz(buf, bsiz);
		}
	}
	buf[len] = '\0';
	fclose(fp);
	*result = buf;
	return len;
}
