/**
 * Please Comply with open source license.
 * @license: MIT License
 * @source: https://github.com/at2er/libsclexer
 */

#ifndef __LIBSCLEXER_H
#define __LIBSCLEXER_H
#include <stdio.h>
#include <stdint.h>

enum SCLEXER_TOK_KIND {
	SCLEXER_TOK_KIND_EMPTY,
	SCLEXER_TOK_KIND_INT,
	SCLEXER_TOK_KIND_INT_NEG, /* negative integer */
	SCLEXER_TOK_KIND_SPACE,
	SCLEXER_TOK_KIND_STR,
	SCLEXER_TOK_KIND_SYM
};

struct sclexer {
	char buf[BUFSIZ], *cur;
	FILE *fp;
	const char *fpath;
	uint64_t line, column;
};

struct sclexer_str_slice {
	char *s;
	uint64_t len;
};

/**
 * @field str:
 *   Value will drop in next line parsing!
 *   If you need use this value, please
 *   create another string to store it.
 */
union sclexer_tok_kind_data {
	char c;
	int64_t sint;
	struct sclexer_str_slice str;
	uint64_t uint;
};

struct sclexer_tok {
	enum SCLEXER_TOK_KIND kind;
	union sclexer_tok_kind_data kind_data;
};

extern void sclexer_end(struct sclexer *lexer);
/**
 * @return:
 *   0: no problem
 *   1: EOF
 */
extern int sclexer_get_line(struct sclexer *lexer);
extern int sclexer_init(const char *fpath, struct sclexer *lexer);
extern int sclexer_read_to(struct sclexer_str_slice *result,
		struct sclexer *lexer,
		const char *s);
extern int sclexer_read_tok(struct sclexer_tok *result, struct sclexer *lexer);
extern int sclexer_read_while(struct sclexer_str_slice *result,
		struct sclexer *lexer,
		int (*cond)(struct sclexer_str_slice *cur,
			struct sclexer *lexer));

#endif
