/**
 * SPDX-License-Identifier: MIT
 */
#ifndef LIBSCLEXER_H
#define LIBSCLEXER_H
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

enum SCLEXER_TOK_KIND {
	SCLEXER_UNKNOWN_TOK,
	SCLEXER_EOF,
	SCLEXER_EOL,

	SCLEXER_IDENT,
	SCLEXER_INT,
	SCLEXER_INT_NEG,
	SCLEXER_KEYWORD,
	SCLEXER_STRING,
	SCLEXER_SYMBOL,

	SCLEXER_INDENT_BLOCK_BEGIN,
	SCLEXER_INDENT_BLOCK_END,

	SCLEXER_TOK_KIND_COUNT
};

struct sclexer_str_slice {
	const char *begin;
	size_t len;
};

struct sclexer_loc {
	const char *fpath;
	size_t line, column;
};

union sclexer_tok_data {
	int64_t  sint;
	uint64_t uint;

	size_t keyword;
	struct sclexer_str_slice str;
	size_t symbol;

	/* it shouldn't be used by user */
	void *_v;
};

struct sclexer_tok {
	union sclexer_tok_data data;
	enum SCLEXER_TOK_KIND kind;

	struct sclexer_str_slice src;
	struct sclexer_loc loc;
};

struct sclexer {
	bool enable_indent;
	bool (*is_ident)(char c, bool begin);

	const char **comments;
	size_t comments_count;
	const char **symbols;
	size_t symbols_count;
	const char **keywords;
	size_t keywords_count;

	const char *src;
	size_t src_siz;

	bool _after_endl;
	const char *_cur;
	size_t _last_ident;
	struct sclexer_loc _loc;
};

bool sclexer_default_is_ident(char c, bool begin);

void sclexer_dup_tok(struct sclexer_tok *dst, struct sclexer_tok *src);

bool sclexer_get_tok(struct sclexer *self,
		struct sclexer_tok *tok);

/**
 * Before calling this function, setup all options without '_xxx' in 'lexer'.
 * If you need read a file, use `sclexer_read_file`
 * and pass the result to this function.
 */
void sclexer_init(struct sclexer *self, const char *fpath);

const char *sclexer_kind_names(enum SCLEXER_TOK_KIND kind);

size_t sclexer_get_tokens(struct sclexer *self, struct sclexer_tok **result);

/**
 * @param result: store the file content, you need free by yourself.
 */
size_t sclexer_read_file(char **result, const char *fpath);

#endif
